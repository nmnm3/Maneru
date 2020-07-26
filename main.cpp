#define _USE_MATH_DEFINES
#include <cmath>
#include <mutex>
#include <random>
#include <algorithm>
#include <windows.h>
#include "resource.h"
#include "game.h"
#include "config.h"
#include "cc.h"
#include "input.h"
#include "graphic.h"

using namespace Maneru;
using std::map;
using std::string;
using std::function;

ControllerInterface *ctrl;
GraphicEngineInterface* engine;
ConfigNode *gameConfig;
ConfigNode *controlConfig;
ConfigNode *graphicConfig;

HANDLE hThread;
bool running = true;
TetrisGame game;

void Fatal(LPCWSTR message)
{
	MessageBox(0, message, 0, 0);
	ExitProcess(0);
}

bool TestGhost(const Tetrimino& ghost, const CCMove& ccm)
{
	const BlockPosition& pos = ghost.GetPosition();
	for (int i = 0; i < 4; i++)
	{
		int x = ccm.expected_x[i];
		int y = ccm.expected_y[i];

		unsigned int mask = pos.at(0, y - ghost.py);

		if ((mask & (1 << (x - ghost.px))) == 0)
			return false;
	}

	return true;
}

bool LineCleared(const CCPlanPlacement& plan)
{
	return plan.cleared_lines[0] != -1;
}

struct BitFieldBag
{
	BitFieldBag(int b = 0) : bits(b) {}
	bool Add(MinoType t)
	{
		int p = (int)t;
		int mask = 1 << p;
		if (bits & mask)
			return false;
		bits |= mask;
		return true;
	}
	bool Remove(MinoType t)
	{
		int p = (int)t;
		int mask = 1 << p;
		if ((bits & mask) == 0)
			return false;
		bits ^= mask;
		return true;
	}
	BitFieldBag& operator+=(const BitFieldBag& other)
	{
		bits |= other.bits;
		return *this;
	}
	int bits;
};

struct NextGenerator7Bag
{
	NextGenerator7Bag() : current(7), bag()
	{
		e.seed(std::random_device()());
		Reset();
	}
	MinoType Next()
	{
		if (current == 7)
		{
			std::shuffle(bag, bag + 7, e);
			current = 0;
		}
		return bag[current++];
	}
	BitFieldBag Remain() const
	{
		BitFieldBag b;
		for (int i = current; i < 7; i++)
		{
			b.Add(bag[i]);
		}
		return b;
	}
	void Reset()
	{
		current = 7;
		for (int i = 0; i < 7; i++)
		{
			bag[i] = (MinoType)i;
		}
	}

	std::default_random_engine e;
	int current;
	MinoType bag[7];
};

class GameLoop
{
public:
	GameLoop()
	{
		initControl();

		gameConfig->GetValue("next", next);
		gameConfig->GetValue("gravity", gravity);
		gameConfig->GetValue("delay_at_beginning", delay);
		gameConfig->GetValue("garbage_min", garbageMin);
		gameConfig->GetValue("garbage_max", garbageMax);
		gameConfig->GetValue("garbage_autolevel", garbageAutoLevel);
		gameConfig->GetValue("hole_repeat", holeRepeat);
		gameConfig->GetValue("exact_cc_move", exactCCMove);
		gameConfig->GetValue("hold_lock", holdLock);
		graphicConfig->GetValue("hint_flash_cycle", hintFlashCycle);
		graphicConfig->GetValue("hint_min_opacity", hintMinOpacity);
		graphicConfig->GetValue("plan_opacity", planOpacity);
		graphicConfig->GetValue("max_plan", maxPlan);

	}

	void Start()
	{
		running = true;
		loop = std::thread(&GameLoop::Run, this);
	}
	void Stop()
	{
		running = false;
		loop.join();
	}

	std::thread loop;

	LARGE_INTEGER freq, last, now;
	int next = 7;
	float gravity = 0.0f;
	int delay = 1000;
	int garbageMin = 5;
	int garbageMax = 10;
	int garbageAutoLevel = 0;
	float holeRepeat = 0.5f;
	bool exactCCMove = true;
	bool holdLock = true;
	float hintFlashCycle = 1.5f;
	float hintMinOpacity = 0.5f;
	float planOpacity = 0.2f;
	int maxPlan = 0;

	CCBot cc;
	NextGenerator7Bag gen;

	ControllerHint ctrlHint;

	bool holdPressed = false;
	int incomingGarbage = 0;
	int combo = 0;
	LARGE_INTEGER lastButtonPress;
	SRSInfo infoCW, infoCCW;

	CCMove ccm;
	int numPlans;
	std::mutex nextLock;

	bool enableColdClear = true;
	bool drawHint = true;

	void pickNext()
	{
		MinoType n = gen.Next();

		game.PushNextPiece(n);
		cc.AddPiece((int)n);
	}

	void updateControlHint()
	{
		const CCPlanPlacement* firstPlan = cc.GetPlan(0);
		if (firstPlan == nullptr) return;
		if (firstPlan->piece == game.GetCurrentPiece().type)
		{
			ctrlHint = game.FindPath(firstPlan->expected_x, firstPlan->expected_y);
		}
		else
		{
			ctrlHint.actions.clear();
			ctrlHint.cost = 0;
		}
	}

	void actionLeft()
	{
		game.MoveLeft();
		QueryPerformanceCounter(&lastButtonPress);
		onPieceMoved();
	}

	void actionRight()
	{
		game.MoveRight();
		QueryPerformanceCounter(&lastButtonPress);
		onPieceMoved();
	}

	void actionSoftDrop()
	{
		game.MoveDown();
		QueryPerformanceCounter(&lastButtonPress);
		onPieceMoved();
	}

	void actionRotateCounterClockwise()
	{
		game.RotateCounterClockwise();
		QueryPerformanceCounter(&lastButtonPress);
		onPieceMoved();
	}

	void actionRotateClockwise()
	{
		game.RotateClockwise();
		QueryPerformanceCounter(&lastButtonPress);
		onPieceMoved();
	}

	void actionGarbage()
	{
		if (incomingGarbage < garbageMin)
		{
			incomingGarbage = garbageMin;
		}
		else if (incomingGarbage >= garbageMax)
		{
			incomingGarbage = garbageMax;
		}
		else
		{
			incomingGarbage++;
		}
	}

	void onPieceMoved()
	{
		game.GetSRSInfo(infoCW, infoCCW);
	}

	void actionResetPiece()
	{
		game.ResetCurrentPiece();
		onPieceMoved();
	}

	void hardReset()
	{
		GameBoard gb;
		game.GetBoard(gb);
		MinoType mt = game.GetHoldPiece();

		int bagPieceRemain = 7 - game.GetPieceIndex() % 7;

		BitFieldBag bag;
		for (int i = 0; i < bagPieceRemain; i++)
		{
			bag.Add(game.GetNextPiece(i));
		}
		if (bagPieceRemain > game.RemainingNext())
		{
			bag += gen.Remain();
		}

		if (mt == PieceNone)
		{
			cc.HardReset(nullptr, gb.field, bag.bits, combo);
		}
		else
		{
			CCPiece p = (CCPiece)mt;
			cc.HardReset(&p, gb.field, bag.bits, combo);
		}

		for (int i = 0; i < game.RemainingNext(); i++)
		{
			cc.AddPiece(game.GetNextPiece(i));
		}
	}

	void softReset()
	{
		GameBoard gb;
		game.GetBoard(gb);
		cc.SoftReset(gb.field, combo);
	}

	void actionHardDrop()
	{
		bool match = TestGhost(game.GetGhost(), ccm);
		if (exactCCMove && enableColdClear)
		{
			if (ccm.hold != holdPressed || !match) return;
		}
		game.LockCurrentPiece();
		bool cleared = game.ClearLines();
		if (cleared) combo++;
		else combo = 0;

		if (incomingGarbage > 0 || !match)
		{
			game.AddGarbage(incomingGarbage, holeRepeat);
			incomingGarbage = 0;

			if (enableColdClear)
			{
				if (!match)
				{
					hardReset();
				}
				else
				{
					softReset();
				}
			}

		}
		if (!cleared && game.GetHighestLine() < garbageAutoLevel)
		{
			incomingGarbage = garbageMin;
		}

		running = game.SpawnCurrentPiece();
		onPieceMoved();
		if (!running)
		{
			Fatal(L"ゲームオーバー");
			return;
		}
		pickNext();
		if (enableColdClear)
		{
			std::unique_lock<std::mutex> l(nextLock);
			numPlans = cc.Next(ccm, incomingGarbage);
			if (numPlans == 0)
			{
				Fatal(L"CCがパニックに陥た");
			}
			updateControlHint();
		}

		QueryPerformanceCounter(&last);
		holdPressed = false;
	}

	void actionHold()
	{
		if (enableColdClear && exactCCMove && holdPressed == ccm.hold)
			return;
		if (holdLock && holdPressed)
			return;
		game.HoldCurrentPiece();
		onPieceMoved();
		holdPressed = !holdPressed;
		if (enableColdClear)
			updateControlHint();
	}

	void actionToggleColdClear()
	{
		if (enableColdClear)
		{
			cc.Stop();
			enableColdClear = false;
			numPlans = 0;
			ccm.depth = 0;
			ccm.nodes = 0;
			ccm.evaluation_result = 0;
			ctrlHint.cost = 0;
			ctrlHint.actions.clear();
		}
		else
		{
			game.PushBackCurrentPiece();
			hardReset();
			enableColdClear = true;
			numPlans = cc.Next(ccm, 0);
			updateControlHint();
			game.SpawnCurrentPiece();
		}
	}

	void actionToggleHint()
	{
		drawHint = !drawHint;
	}

	void initControl()
	{
		map<string, function<void(void)>> actions;
		actions["left"] = [this]() { this->actionLeft(); };
		actions["right"] = [this]() { this->actionRight(); };
		actions["soft_drop"] = [this]() { this->actionSoftDrop(); };
		actions["rotate_counter_clockwise"] = [this]() { this->actionRotateCounterClockwise(); };
		actions["rotate_clockwise"] = [this]() { this->actionRotateClockwise(); };
		actions["garbage"] = [this]() { this->actionGarbage(); };
		actions["reset_piece"] = [this]() { this->actionResetPiece(); };
		actions["hard_drop"] = [this]() { this->actionHardDrop(); };
		actions["hold"] = [this]() { this->actionHold(); };
		actions["toggle_cc"] = [this]() { this->actionToggleColdClear(); };
		actions["toggle_hint"] = [this]() { this->actionToggleHint(); };

		map<string, string> mapping =
		{
			{"left", "left"},
			{"right", "right"},
			{"up", "hard_drop"},
			{"down", "soft_drop" },
			{"a", "rotate_counter_clockwise"},
			{"x", "rotate_counter_clockwise"},
			{"b", "rotate_clockwise"},
			{"y", "rotate_clockwise"},
			{"start", "garbage"},
			{"lb", "hold"},
			{"rb", "hold"},
			{"ls", "reset_piece"},
			{"rs", "toggle_cc"},
		};

		for (const auto& m : mapping)
		{
			string action = m.second;
			if (!controlConfig->GetValue(m.first, action))
				continue;
			auto it = actions.find(action);
			if (it != actions.end())
			{
				ConfigNode ratecfg = controlConfig->GetNode(m.first);
				float repeatDelay = 0;
				float repeatRate = 0;
				ratecfg.GetValue("repeat_delay", repeatDelay);
				ratecfg.GetValue("repeat_rate", repeatRate);
				ctrl->SetButtonFunction(m.first.c_str(), it->second, repeatDelay, repeatRate);
			}
		}
	}

	void Run()
	{
		game.SetGravity(gravity);
		if (gravity >= 20.0f)
			cc.Start(true);
		else
			cc.Start(false);

		QueryPerformanceFrequency(&freq);

		for (int i = 0; i < next; i++)
			pickNext();

		engine->StartDraw();
		engine->DrawBoard(game);
		engine->DrawCurrentPiece(game.GetCurrentPiece());
		engine->DrawNextPreview(game);
		engine->DrawHold(PieceNone, false);
		engine->FinishDraw();
		Sleep(delay);

		numPlans = cc.Next(ccm, 0);
		updateControlHint();
		QueryPerformanceCounter(&last);

		game.SpawnCurrentPiece();
		onPieceMoved();
		WCHAR strNodes[0x20];
		WCHAR strValue[0x20];
		WCHAR strPieceIndex[0x20];
		WCHAR strCombo[0x20];
		WCHAR strGarbage[0x20];
		WCHAR strState[0x20];
		WCHAR strCW[0x20];
		WCHAR strCCW[0x20];
		WCHAR strMoveCost[0x20];
		LPCWSTR stats[32] =
		{
			strNodes,
			strValue,
			strPieceIndex,
			strCombo,
			strGarbage,
			strState,
			strCW,
			strCCW,
			strMoveCost,
		};
		const int fixedStats = 9;
		while (running)
		{
			QueryPerformanceCounter(&now);
			float diff = (now.QuadPart - last.QuadPart) / float(freq.QuadPart);
			float alpha = (sin(diff / hintFlashCycle * (2 * M_PI)) + 1) / 2;
			alpha = alpha * (1 - hintMinOpacity) + hintMinOpacity;
			engine->StartDraw();
			engine->DrawBoard(game);
			engine->DrawCurrentPiece(game.GetCurrentPiece());
			engine->DrawNextPreview(game);
			if (holdLock)
				engine->DrawHold(game.GetHoldPiece(), holdPressed);
			else
				engine->DrawHold(game.GetHoldPiece(), false);

			if (enableColdClear && drawHint)
			{
				if (holdPressed != ccm.hold)
					engine->DrawHoldHint(alpha);

				// Lock for plans. Don't draw if CC is updating the plans.
				{
					std::unique_lock<std::mutex> l(nextLock);
					const CCPlanPlacement* p = cc.GetPlan(0);
					engine->DrawHint((unsigned char*)p->expected_x, (unsigned char*)p->expected_y, PieceHint, alpha);
					int n = numPlans;
					if (maxPlan > 0 && n > maxPlan)
					{
						n = maxPlan;
					}
					for (int i = 1; i < n; i++)
					{
						alpha = (1 - planOpacity) / i + planOpacity;
						p = cc.GetPlan(i);
						engine->DrawHint((unsigned char*)p->expected_x, (unsigned char*)p->expected_y, (MinoType)p->piece, alpha);
					}

					float sinceButtonPress = (now.QuadPart - lastButtonPress.QuadPart) / float(freq.QuadPart);
					if (sinceButtonPress > 1.5)
					{
						updateControlHint();
						lastButtonPress.QuadPart = now.QuadPart;
					}
				}
			}

			wsprintf(strNodes, L"ノード数: %d", ccm.nodes);
			wsprintf(strValue, L"評価値: %d", ccm.evaluation_result);
			wsprintf(strPieceIndex, L"7種残り: %d", 7 - game.GetPieceIndex() % 7);

			if (combo > 1)
				wsprintf(strCombo, L"REN: %d", combo - 1);
			else
				strCombo[0] = '\0';

			wsprintf(strGarbage, L"受ける火力: %d", incomingGarbage);
			wsprintf(strState, L"向き: %d", game.GetCurrentPiece().state);
			if (infoCW.index >= 0)
				wsprintf(strCW, L"A/Y: %d(%d,%d)", infoCW.index, infoCW.x, infoCW.y);
			else
				wcscpy(strCW, L"A/Y: 不可能");
			if (infoCCW.index >= 0)
				wsprintf(strCCW, L"B/X: %d(%d,%d)", infoCCW.index, infoCCW.x, infoCCW.y);
			else
				wcscpy(strCCW, L"B/X: 不可能");
			wsprintf(strMoveCost, L"コスト: %d", ctrlHint.cost);
			int statIndex = fixedStats;

			for (auto it = ctrlHint.actions.rbegin(); it != ctrlHint.actions.rend(); it++)
			{
				stats[statIndex] = GetControllerActionDescription(*it);
				statIndex++;
			}

			engine->DrawStats(stats, statIndex);

			engine->FinishDraw();
			ctrl->Refresh();
		}
	}
};

void CenterWindow(HWND h)
{
	HWND d = GetDesktopWindow();
	RECT rd;
	GetWindowRect(d, &rd);
	RECT rc;
	GetWindowRect(h, &rc);
	LONG width = rc.right - rc.left;
	LONG height = rc.bottom - rc.top;
	LONG left = (rd.right - width) / 2;
	LONG top = (rd.bottom - height) / 2;
	MoveWindow(h, left, top, width, height, 1);
}

INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND list;
	switch (message)
	{
	case WM_INITDIALOG:
	{
		CenterWindow(hwndDlg);
		std::vector<string> *ctrls = (std::vector<string>*) lParam;
		list = GetDlgItem(hwndDlg, IDC_LIST1);
		WCHAR name[0x100];
		for (const string& ctrl : *ctrls)
		{
			int n = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ctrl.c_str(), ctrl.size(), name, sizeof(name));
			if (n > 0)
			{
				name[n] = 0;
				SendMessage(list, LB_ADDSTRING, 0, (LPARAM)name);
			}
		}
		SendMessage(list, LB_SETCURSEL, 0, 0);
		return TRUE;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK)
		{
			list = GetDlgItem(hwndDlg, IDC_LIST1);
			int index = SendMessage(list, LB_GETCURSEL, 0, 0);
			if (index >= 0)
				EndDialog(hwndDlg, index);
		}
		return TRUE;
	}
	default:
		return FALSE;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!running)
	{
		PostQuitMessage(0);
	}
	switch (message)
	{
	case WM_DESTROY:
		running = false;
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

const int BASE_WIDTH = 600;
const int BASE_HEIGHT = 400;

float GetResolutionMagnifyFactor()
{
	string resolution = "600x400";
	graphicConfig->GetValue("resolution", resolution);
	if (resolution == "1800x1200")
	{
		return 3.0f;
	}
	if (resolution == "1200x800")
	{
		return 2.0f;
	}
	if (resolution == "900x600")
	{
		return 1.5f;
	}
	return 1.0f;
}

void SetupController(HINSTANCE hInstance)
{
	ControllerList list = GetControllerList();
	if (list.size() == 0)
	{
		Fatal(L"コントローラーが見つかりません");
	}

	std::vector<std::string> names;
	for (const auto& p : list)
	{
		names.push_back(p.first);
	}
	int index = 0;
	if (list.size() > 1)
	{
		index = DialogBoxParam(hInstance, (LPCWSTR)IDD_DIALOG1, 0, DlgProc, (LPARAM)&names);
	}
	index = list[names[index]];
	ctrl = OpenController(index);
	if (ctrl == nullptr)
	{
		Fatal(L"コントローラー初期化失敗");
	}
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	if (!LoadConfig("maneru.config"))
	{
		Fatal(L"コンフィグ読み込み失敗");
	}
	SetupController(hInstance);

	ConfigNode *global = GetGlobalConfig();
	ConfigNode gameConfig = global->GetNode("game");
	ConfigNode controlConfig = global->GetNode("control");
	ConfigNode graphicConfig = global->GetNode("graphic");
	::gameConfig = &gameConfig;
	::controlConfig = &controlConfig;
	::graphicConfig = &graphicConfig;

	// the handle for the window, filled by a function
	HWND hWnd;
	// this struct holds information for the window class
	WNDCLASSEX wc;

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"Maneru";

	// register the window class
	RegisterClassEx(&wc);
	float magnify = GetResolutionMagnifyFactor();

	int x = 0;
	int y = 0;
	int w = BASE_WIDTH * magnify;
	int h = BASE_HEIGHT * magnify;
	// create the window and use the result as the handle
	hWnd = CreateWindowEx(NULL,
		L"Maneru",    // name of the window class
		L"まねるちゃん - Cold Clear",   // title of the window
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,    // window style
		x,    // x-position of the window
		y,    // y-position of the window
		w,    // width of the window
		h,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		hInstance,    // application handle
		NULL);    // used with multiple windows, NULL
	RECT rect;
	GetClientRect(hWnd, &rect);
	int wdiff = w - rect.right;
	int hdiff = h - rect.bottom;
	MoveWindow(hWnd, x, y, w + wdiff, h + hdiff, 1);
	CenterWindow(hWnd);
	ShowWindow(hWnd, nCmdShow);

	int fps = 60;
	graphicConfig.GetValue("fps", fps);
	engine = InitGraphicEngine(hWnd, magnify, fps);
	if (engine == NULL)
	{
		Fatal(L"DirectX初期化が失敗しました");
	}

	GameLoop gl;
	gl.Start();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	gl.Stop();
	engine->Shutdown();

	ExitProcess(0);
}
