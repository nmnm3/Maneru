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
TetrisGame globalgame;

HANDLE hThread;
bool running = true;
TetrisGame game;
MinoType bag[7] =
{
	PieceI,
	PieceT,
	PieceO,
	PieceS,
	PieceZ,
	PieceL,
	PieceJ,
};

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

DWORD WINAPI GameLoop(PVOID)
{
	ConfigNode *global = GetGlobalConfig();
	ConfigNode gameConfig = global->GetNode("game");
	ConfigNode controlConfig = global->GetNode("control");
	ConfigNode graphicConfig = global->GetNode("graphic");
	LARGE_INTEGER freq, last, now;

	int next = 7;
	int delay = 1000;
	int garbageMin = 5;
	int garbageMax = 10;
	float holeRepeat = 0.5f;

	gameConfig.GetValue("next", next);
	gameConfig.GetValue("delay_at_beginning", delay);
	gameConfig.GetValue("garbage_min", garbageMin);
	gameConfig.GetValue("garbage_max", garbageMax);
	gameConfig.GetValue("hole_repeat", holeRepeat);


	int fps = 60;
	float hintFlashCycle = 1.5f;
	float hintMinOpacity = 0.5f;
	float planOpacity = 0.2f;
	graphicConfig.GetValue("fps", fps);
	graphicConfig.GetValue("hint_flash_cycle", hintFlashCycle);
	graphicConfig.GetValue("hint_min_opacity", hintMinOpacity);
	graphicConfig.GetValue("plan_opacity", planOpacity);

	engine->SetFPS(fps);
	QueryPerformanceFrequency(&freq);
	srand(GetTickCount());
	CCBot cc;

	int currentBagPiece = 7;
	auto pickNext = [&]() {
		if (currentBagPiece == 7)
		{
			std::random_shuffle(std::begin(bag), std::end(bag));
			currentBagPiece = 0;
		}
		game.PushNextPiece(bag[currentBagPiece]);
		cc.AddPiece(bag[currentBagPiece]);
		currentBagPiece++;
	};
	for (int i = 0; i < next; i++)
		pickNext();

	CCMove ccm;

	engine->StartDraw();
	engine->DrawScreen(game);
	engine->FinishDraw();
	Sleep(delay);

	CCPlan plan = cc.Next(ccm);
	std::mutex nextLock;
	QueryPerformanceCounter(&last);
	bool holdPressed = false;
	int incomingGarbage = 0;
	map<string, function<void(void)>> actions;

	actions["left"] = []() {game.MoveLeft(); };
	actions["right"] = []() {game.MoveRight(); };
	actions["soft_drop"] = []() {game.MoveDown(); };
	actions["rotate_counter_clockwise"] = []() {game.RotateCounterClockwise(); };
	actions["rotate_clockwise"] = []() {game.RotateClockwise(); };
	actions["garbage"] = [&]() {
		if (incomingGarbage < garbageMin) incomingGarbage = garbageMin;
		else if (incomingGarbage >= garbageMax) incomingGarbage = garbageMax;
		else incomingGarbage++;
	};
	actions["hard_drop"] = [&]() {
		if (ccm.hold != holdPressed) return;
		if (!TestGhost(game.GetGhost(), ccm)) return;
		game.LockCurrentPiece();
		game.ClearLines();

		if (incomingGarbage > 0)
		{
			game.AddGarbage(incomingGarbage, holeRepeat);
			incomingGarbage = 0;
			GameBoard gb = game.GetBoard();
			cc.ResetBot(gb.field);
		}
		running = game.SpawnCurrentPiece();
		if (!running)
		{
			MessageBox(0, L"ゲームオーバー", L"ゲームオーバー", 0);
			return;
		}
		pickNext();
		std::unique_lock<std::mutex> l(nextLock);
		plan = cc.Next(ccm);
		QueryPerformanceCounter(&last);
		holdPressed = false;
	};

	actions["hold"] = [&holdPressed, &ccm]() {
		if (holdPressed == ccm.hold) return;
		game.HoldCurrentPiece();
		holdPressed = !holdPressed;
	};

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
	};

	for (const auto& m : mapping)
	{
		string action = m.second;
		if (!controlConfig.GetValue(m.first, action))
			continue;
		auto it = actions.find(action);
		if (it != actions.end())
		{
			ConfigNode ratecfg = controlConfig.GetNode(m.first);
			float repeatDelay = 0;
			float repeatRate = 0;
			ratecfg.GetValue("repeat_delay", repeatDelay);
			ratecfg.GetValue("repeat_rate", repeatRate);
			ctrl->SetButtonFunction(m.first.c_str(), it->second, repeatDelay, repeatRate);
		}
	}

	game.SpawnCurrentPiece();
	WCHAR statNodes[0x20];
	WCHAR statDepth[0x10];
	WCHAR statRank[0x10];
	WCHAR garbage[0x10];
	LPCWSTR stats[] = { statNodes, statDepth, statRank, garbage };
	while (running)
	{
		QueryPerformanceCounter(&now);
		float diff = (now.QuadPart - last.QuadPart) / float(freq.QuadPart);
		float alpha = (sin(diff / hintFlashCycle * (2 * M_PI)) + 1) / 2;
		alpha = alpha * (1 - hintMinOpacity) + hintMinOpacity;
		engine->StartDraw();
		engine->DrawScreen(game);
		if (holdPressed != ccm.hold)
		{
			engine->DrawHoldHint(alpha);
		}

		{
			std::unique_lock<std::mutex> l(nextLock);
			CCPlanPlacement* p = plan.plans;
			engine->DrawHint((unsigned char*)p->expected_x, (unsigned char*)p->expected_y, PieceHint, alpha);

			for (int i = 1; i < plan.n; i++)
			{
				alpha = (1 - planOpacity) / i + planOpacity;
				p = plan.plans + i;
				engine->DrawHint((unsigned char*)p->expected_x, (unsigned char*)p->expected_y, (MinoType)p->piece, alpha);
			}
		}

		wsprintf(statNodes, L"nodes: %d", ccm.nodes);
		wsprintf(statDepth, L"depth: %d", ccm.depth);
		wsprintf(statRank, L"rank: %d", ccm.original_rank);
		wsprintf(garbage, L"garbage: %d", incomingGarbage);
		engine->DrawStats(stats, ARRAYSIZE(stats));

		engine->FinishDraw();
		ctrl->Refresh();
	}
	return 0;
}

void StartGame()
{
	running = true;
	hThread = CreateThread(0, 0, GameLoop, 0, 0, 0);
}
void StopGame()
{
	running = false;
	WaitForSingleObject(hThread, -1);
	CloseHandle(hThread);
}

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
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	if (!LoadConfig("maneru.config"))
	{
		MessageBox(0, L"コンフィグ読み込み失敗", L"", 0);
		ExitProcess(0);
	}
	std::vector<string> list = GetControllerList();
	if (list.size() == 0)
	{
		MessageBox(0, L"コントローラーが見つかりません", L"", 0);
		ExitProcess(0);
	}
	int index = 0;
	if (list.size() > 1)
	{
		index = DialogBoxParam(hInstance, (LPCWSTR)IDD_DIALOG1, 0, DlgProc, (LPARAM)&list);
	}
	ctrl = OpenController(index);
	CoInitialize(0);
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

	int x = 0;
	int y = 0;
	int w = 600;
	int h = 400;
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

	engine = InitGraphicEngine(hWnd);
	if (engine == NULL)
	{
		MessageBox(0, L"DirectX初期化が失敗しました", 0, 0);
		ExitProcess(0);
	}

	StartGame();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	StopGame();
	engine->Shutdown();

	ExitProcess(0);
}
