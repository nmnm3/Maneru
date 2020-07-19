#include <windows.h>
#include <random>
#include "game.h"
#include "input.h"
using namespace Maneru;

const BlockPosition masks[7][4] = {
	{ //I
		{0, 0, 0xF, 0},
		{4, 4, 4, 4},
		{0, 0xF, 0, 0},
		{2, 2, 2, 2},
	},
	{ //O
		{0, 6, 6, 0},
		{0, 6, 6, 0},
		{0, 6, 6, 0},
		{0, 6, 6, 0},
	},
	{ //T
		{0, 7, 2, 0},
		{2, 6, 2, 0},
		{2, 7, 0, 0},
		{2, 3, 2, 0},
	},
	{ //L
		{0, 7, 4, 0},
		{6, 2, 2, 0},
		{1, 7, 0, 0},
		{2, 2, 3, 0},
	},
	{ //J
		{0, 7, 1, 0},
		{2, 2, 6, 0},
		{4, 7, 0, 0},
		{3, 2, 2, 0},
	},
	{ //S
		{0, 3, 6, 0},
		{4, 6, 2, 0},
		{3, 6, 0, 0},
		{2, 3, 1, 0},
	},
	{ //Z
		{0, 6, 3, 0},
		{2, 6, 4, 0},
		{6, 3, 0, 0},
		{1, 3, 2, 0},
	}
};

struct SRSTestPair
{
	int x, y;
};

// https://harddrop.com/wiki/SRS
const SRSTestPair srsMatrixClockwiseForI[4][5] =
{
	{ {0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2} }, // 0->R
	{ {0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1} }, // R->2
	{ {0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2} }, // 2->L
	{ {0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1} }, // L->0
};

const SRSTestPair srsMatrixClockwiseForTSZLJ[4][5] =
{
	{ {0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2} }, // 0->R
	{ {0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2} }, // R->2
	{ {0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2} }, // 2->L
	{ {0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2} }, // L->0
};

bool Tetrimino::Test(const LineRect& rect) const
{
	const BlockPosition& mask = masks[type][state];
	if (px < 0)
	{
		// Check left boarder
		int m = (1 << (-px)) - 1;
		if (mask[3] & m) return false;
		if (mask[2] & m) return false;
		if (mask[1] & m) return false;
		if (mask[0] & m) return false;
	}
	else if (px >= BOARD_WIDTH - 3)
	{
		// Check right boarder
		int m = (~((1 << BOARD_WIDTH) - 1)) >> px;
		if (mask[3] & m) return false;
		if (mask[2] & m) return false;
		if (mask[1] & m) return false;
		if (mask[0] & m) return false;
	}

	// Check bottom
	if (py < -2) return false;
	if (py == -2)
	{
		if (mask[0]) return false;
		if (mask[1]) return false;
	}
	if (py == -1)
	{
		if (mask[0]) return false;
	}

	// Check exist pieces
	if (mask.at(px, 3) & rect.lines[3]) return false;
	if (mask.at(px, 2) & rect.lines[2]) return false;
	if (mask.at(px, 1) & rect.lines[1]) return false;
	if (mask.at(px, 0) & rect.lines[0]) return false;
	return true;
}

const BlockPosition& Tetrimino::GetPosition() const
{
	return masks[type][state];
}

TetrisGame::TetrisGame()
{
	memset(board, 0, sizeof(board));
	for (int i = 0; i < 40; i++)
	{
		board[i] = 0;
		for (int j = 0; j < 10; j++)
			boardColor[i][j] = PieceNone;
	}
	hold = PieceNone;
	current.type = PieceNone;
	pieceIndex = 0;
	gravity = 0;
}

void TetrisGame::PushNextPiece(MinoType t)
{
	next.push_back(t);
}

void Maneru::TetrisGame::PushBackCurrentPiece()
{
	next.push_front(current.type);
	pieceIndex--;
}

const Tetrimino & TetrisGame::GetCurrentPiece() const
{
	return current;
}

MinoType TetrisGame::GetNextPiece(int index) const
{
	return next[index];
}

MinoType TetrisGame::GetHoldPiece() const
{
	return hold;
}

MinoType TetrisGame::PopNextPiece()
{
	MinoType piece = next.front();
	next.pop_front();
	pieceIndex++;
	return piece;
}

void TetrisGame::LockCurrentPiece()
{
	if (current.type != PieceNone)
	{
		LineRect rect;
		rect.lines = board + current.py;
		while(current.Test(rect))
		{
			rect.lines--;
			current.py--;
		}
		current.py++;

		const BlockPosition& pos = current.GetPosition();
		for (int y = 0; y < 4; y++)
		{
			if (current.py + y >= 0)
			{
				unsigned int occupied = pos.at(current.px, y);
				board[current.py + y] |= occupied;
				for (int x = 0; x < 4; x++)
				{
					if (pos.at(0, y) & (1 << x))
					{
						if (current.px + x >= BOARD_WIDTH)
						{
							__debugbreak();
						}
						boardColor[current.py + y][current.px + x] = current.type;
					}
				}
			}
		}
	}
}

Tetrimino TetrisGame::GetGhost() const
{
	Tetrimino ghost = current;
	LineRect rect;
	rect.lines = board + ghost.py;
	while (ghost.Test(rect))
	{
		rect.lines--;
		ghost.py--;
	}
	ghost.py++;
	return ghost;
}

bool TetrisGame::SpawnCurrentPiece()
{
	current.type = next.front();
	current.px = 3;
	current.py = VISIBLE_LINES - 3;
	current.state = 0;
	next.pop_front();
	pieceIndex++;

	LineRect rect;
	rect.lines = board + current.py;
	bool alive = current.Test(rect);
	if (alive && Is20G()) FastDrop();
	return alive;
}

void Maneru::TetrisGame::ResetCurrentPiece()
{
	current.px = 3;
	current.py = VISIBLE_LINES - 3;
	current.state = 0;
	if (Is20G()) FastDrop();
}

MinoType TetrisGame::HoldCurrentPiece()
{
	current.px = 3;
	current.py = VISIBLE_LINES - 3;
	current.state = 0;
	if (hold != PieceNone)
	{
		MinoType t = hold;
		hold = current.type;
		current.type = t;
		if (Is20G()) FastDrop();
		return t;
	}
	else
	{
		hold = current.type;
		current.type = PopNextPiece();
		if (Is20G()) FastDrop();
		return current.type;
	}
}

bool TetrisGame::MoveLeft()
{
	int px = current.px;
	current.px--;
	LineRect rect;
	rect.lines = board + current.py;
	if (!current.Test(rect))
	{
		current.px = px;
		return false;
	}
	if (Is20G()) FastDrop();
	return true;
}

bool TetrisGame::MoveRight()
{
	int px = current.px;
	current.px++;
	LineRect rect;
	rect.lines = board + current.py;
	if (!current.Test(rect))
	{
		current.px = px;
		return false;
	}
	if (Is20G()) FastDrop();
	return true;
}

bool TetrisGame::MoveDown()
{
	int py = current.py;
	current.py--;
	LineRect rect;
	rect.lines = board + current.py;
	if (!current.Test(rect))
	{
		current.py = py;
		return false;
	}
	if (Is20G()) FastDrop();
	return true;
}

bool TetrisGame::RotateClockwise()
{
	int lastState = current.state;
	current.state++;
	if (current.state == 4) current.state = 0;
	// Tests for Super Rotation System
	int lastx = current.px;
	int lasty = current.py;
	switch (current.type)
	{
	case PieceI:
		for (int i = 0; i < 5; i++)
		{
			SRSTestPair pair = srsMatrixClockwiseForI[lastState][i];
			current.px = lastx + pair.x;
			current.py = lasty + pair.y;
			LineRect rect;
			rect.lines = board + current.py;
			if (current.Test(rect))
			{
				if (Is20G()) FastDrop();
				return true;
			}
		}
		break;
	case PieceO:
		return true;
	default:
		//TSZLJ
		for (int i = 0; i < 5; i++)
		{
			SRSTestPair pair = srsMatrixClockwiseForTSZLJ[lastState][i];
			current.px = lastx + pair.x;
			current.py = lasty + pair.y;
			LineRect rect;
			rect.lines = board + current.py;
			if (current.Test(rect))
			{
				if (Is20G()) FastDrop();
				return true;
			}
		}

		break;
	}
	// Rotation failed
	current.state = lastState;
	current.px = lastx;
	current.py = lasty;
	return false;
}

bool TetrisGame::RotateCounterClockwise()
{
	int lastState = current.state;
	current.state--;
	if (current.state < 0) current.state = 3;

	// Tests for Super Rotation System
	int lastx = current.px;
	int lasty = current.py;
	switch (current.type)
	{
	case PieceI:
		for (int i = 0; i < 5; i++)
		{
			SRSTestPair pair = srsMatrixClockwiseForI[current.state][i];
			current.px = lastx - pair.x;
			current.py = lasty - pair.y;
			LineRect rect;
			rect.lines = board + current.py;
			if (current.Test(rect))
			{
				if (Is20G()) FastDrop();
				return true;
			}
		}
		break;
	case PieceO:
		return false;
	default:
		//TSZLJ
		for (int i = 0; i < 5; i++)
		{
			SRSTestPair pair = srsMatrixClockwiseForTSZLJ[current.state][i];
			current.px = lastx - pair.x;
			current.py = lasty - pair.y;
			LineRect rect;
			rect.lines = board + current.py;
			if (current.Test(rect))
			{
				if (Is20G()) FastDrop();
				return true;
			}
		}
		break;
	}
	// Rotation failed
	current.state = lastState;
	current.px = lastx;
	current.py = lasty;
	return false;
}

void Maneru::TetrisGame::SetGravity(float gravity)
{
	this->gravity = gravity;
}

void TetrisGame::SetColor(int x, int y, MinoType color)
{
	boardColor[y][x] = color;
	board[y] |= 1 << x;
}

bool TetrisGame::ClearLines()
{
	bool cleared = false;
	int index = 0;
	while (board[index])
	{
		if (board[index] == 0x3FF)
		{
			cleared = true;
			board[index] = 0;
			for (int j = 0; j < 10; j++)
			{
				boardColor[index][j] = PieceNone;
			}
			for (int i = index; board[i + 1]; i++)
			{
				board[i] = board[i + 1];
				board[i + 1] = 0;
				for (int j = 0; j < 10; j++)
				{
					boardColor[i][j] = boardColor[i + 1][j];
					boardColor[i + 1][j] = PieceNone;
				}
			}
		}
		else index++;
	}
	return cleared;
}

int Maneru::TetrisGame::GetHighestLine() const
{
	int l = 0;
	while (this->board[l] != 0) l++;
	return l - 1;
}

void TetrisGame::AddGarbage(int lines, float holeRepeat)
{
	if (lines == 0) return;

	std::default_random_engine e;
	e.seed(std::random_device()());
	std::uniform_real_distribution<> distRepeat(0.0, 1.0);
	std::uniform_int_distribution<> distPos(0, 9);
	int holes[20];
	int current = 0;
	bool same = false;
	while (current < lines)
	{
		if (same)
		{
			holes[current] = holes[current - 1];
		}
		else
		{
			holes[current] = distPos(e);
		}
		same = distRepeat(e) < holeRepeat;
		current++;
	}
	int highest = 0;
	while (board[highest]) highest++;
	highest--;
	while (highest >= 0)
	{
		board[highest + lines] = board[highest];
		for (int i = 0; i < 10; i++)
		{
			boardColor[highest + lines][i] = boardColor[highest][i];
		}
		highest--;
	}
	for (int i = 0; i < lines; i++)
	{
		board[lines - 1 - i] = 0x3FF ^ (1 << holes[i]);
		for (int j = 0; j < 10; j++)
		{
			boardColor[lines - 1 - i][j] = PieceGarbage;
		}
		boardColor[lines - 1 - i][holes[i]] = PieceNone;
	}
}

MinoType TetrisGame::GetColor(int x, int y) const
{
	return boardColor[x][y];
}

int TetrisGame::RemainingNext() const
{
	return next.size();
}

int Maneru::TetrisGame::GetPieceIndex() const
{
	return pieceIndex;
}

void TetrisGame::GetBoard(GameBoard &b) const
{
	for (int y = 0; y < 40; y++)
	{
		unsigned int line = board[y];
		for (int x = 0; x < 10; x++)
		{
			b.field[y * 10 + x] = (line & (1 << x)) > 0;
		}
	}
}

struct PathNode
{
	int cost;
	ControllerAction action;
	int from;
};

constexpr int d1 = BOARD_WIDTH + 2;
constexpr int d2 = VISIBLE_LINES + 2;
constexpr int d3 = 4;
constexpr int totalPositions = d1 * d2 * d3;

struct PathPosition
{
	int x, y;
	int state;
	PathPosition(const Tetrimino& c)
	{
		x = c.px;
		y = c.py;
		state = c.state;
	}
	void Set(Tetrimino& c) const
	{
		c.px = x;
		c.py = y;
		c.state = state;
	}
	bool Match(const unsigned char expectedX[4], const unsigned char expectedY[4], MinoType t) const
	{
		const unsigned char* m = masks[t][state].mask;
		for (int i = 0; i < 4; i++)
		{
			int xx = expectedX[i] - x;
			int yy = expectedY[i] - y;
			if (xx < 0 || xx >= 4) return false;
			if (yy < 0 || yy >= 4) return false;
			if ((m[yy] & (1 << xx)) == 0) return false;
		}
		return true;
	}
	int PathIndex() const
	{
		int index = x + 2;
		index *= d2;
		index += y + 2;
		index *= d3;
		index += state;
		return index;
	}
};

typedef bool(TetrisGame::* SimpleActionFunction)();
struct ActionTableEntry
{
	ControllerAction action;
	SimpleActionFunction fn;
	int cost;
	int distanceMultiplier;
};

ControllerHint Maneru::TetrisGame::FindPath(const unsigned char expectedX[4], const unsigned char expectedY[4])
{
	const static ActionTableEntry table[] =
	{
		{ControllerAction::Left, &TetrisGame::MoveLeft, 20, 0},
		{ControllerAction::Right, &TetrisGame::MoveRight, 20, 0},
		{ControllerAction::Down, &TetrisGame::MoveDown, 100, 0},
		{ControllerAction::RotateClockwise, &TetrisGame::RotateClockwise, 20, 0},
		{ControllerAction::RotateCounterClockwise, &TetrisGame::RotateCounterClockwise, 20, 0},
		{ControllerAction::FastLeft, &TetrisGame::FastLeft, 15, 1},
		{ControllerAction::FastRight, &TetrisGame::FastRight, 15, 1},
		{ControllerAction::FastDrop, &TetrisGame::FastDrop, 20, 2},
	};

	Tetrimino back = current;
	static PathNode positions[totalPositions];
	memset(positions, 0, sizeof(positions));

	std::queue<PathPosition> q;
	q.push(current);
	{
		PathNode& node = positions[q.front().PathIndex()];
		node.from = -1;
		node.cost = 10;
		node.action = ControllerAction::None;
	}

	int bestIndex = -1;
	while (!q.empty())
	{
		const PathPosition& start = q.front();
		start.Set(current);
		FastDrop();
		PathPosition afterHardDrop(current);
		if (afterHardDrop.Match(expectedX, expectedY, current.type))
		{
			int index = start.PathIndex();
			if (bestIndex == -1 || positions[index].cost < positions[bestIndex].cost)
			{
				bestIndex = index;
			}
		}
		int startIndex = start.PathIndex();
		for (auto& entry : table)
		{
			start.Set(current);
			int startx = current.px;
			int starty = current.py;
			if ((this->*entry.fn)())
			{
				PathPosition end(current);
				int cost = positions[startIndex].cost + entry.cost;
				if (entry.distanceMultiplier != 0)
				{
					int diff = startx - current.px;
					if (diff < 0) diff = -diff;
					cost += diff * entry.distanceMultiplier;
					diff = starty - current.py;
					if (diff < 0) diff = -diff;
					cost += diff * entry.distanceMultiplier;
				}

				int endIndex = end.PathIndex();
				PathNode& node = positions[endIndex];
				if (node.cost == 0 || node.cost > cost)
				{
					node.cost = cost;
					node.action = entry.action;
					node.from = startIndex;
					q.push(end);
				}
			}
		}
		q.pop();
	}

	current = back;
	ControllerHint hint;
	if (bestIndex != -1)
	{
		hint.cost = positions[bestIndex].cost;
		hint.actions.push_back(ControllerAction::HardDrop);
		int index = bestIndex;
		while (positions[index].from != -1)
		{
			hint.actions.push_back(positions[index].action);
			index = positions[index].from;
		}
	}
	else
	{
		hint.cost = 0;
	}
	return hint;
}

int Maneru::TetrisGame::FindPathIndex() const
{
	int d1 = BOARD_WIDTH + 2;
	int d2 = VISIBLE_LINES + 4;
	int d3 = 4;
	int index = current.px + 2;
	index *= d1;
	index += current.py + 3;
	index *= d2;
	index += current.state;
	return index;
}

bool Maneru::TetrisGame::FastDrop()
{
	int lasty = current.py;
	LineRect rect;
	rect.lines = board + current.py;
	while (current.Test(rect))
	{
		rect.lines--;
		current.py--;
	}
	current.py++;

	return current.py != lasty;
}

bool Maneru::TetrisGame::FastLeft()
{
	if (!MoveLeft()) return false;
	while (MoveLeft());
	return true;
}

bool Maneru::TetrisGame::FastRight()
{
	if (!MoveRight()) return false;
	while (MoveRight());
	return true;
}

bool Maneru::TetrisGame::Is20G() const
{
	return gravity >= 20.0;
}

unsigned int BlockPosition::at(int x, int y) const
{
	if (x >= 0)
	{
		return mask[y] << x;
	}
	else
	{
		return mask[y] >> (-x);
	}
}

unsigned int BlockPosition::operator[](int index) const
{
	return mask[index];
}

static const wchar_t* descrptions[] =
{
	L"None",
	L"←",
	L"→",
	L"↓",
	L"A/Y",
	L"B/X",
	L"← ++",
	L"→ ++",
	L"↓ ++",
	L"↑",
};

const wchar_t* Maneru::GetControllerActionDescription(ControllerAction act)
{
	return descrptions[(int)act];
}
