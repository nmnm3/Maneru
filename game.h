#pragma once
#include <mutex>
#include <queue>

namespace Maneru
{
enum MinoType
{
	PieceI,
	PieceO,
	PieceT,
	PieceL,
	PieceJ,
	PieceS,
	PieceZ,
	PieceGarbage,
	PieceNone,
	PieceHint,
};

constexpr int VISIBLE_LINES = 20;
constexpr int BOARD_WIDTH = 10;

struct LineRect
{
	const unsigned int *lines; // 4 consecutive lines.
};

struct BlockPosition
{
	unsigned char mask[4]; // One for each line.
	unsigned int at(int x, int y) const;
	unsigned int operator[](int index) const;
};

struct Tetrimino
{
	int px, py; // Bottom left corner of the piece rect.
	MinoType type; // Piece category.
	int state; // one of 4 states.
	bool Test(const LineRect& rect) const;
	const BlockPosition& GetPosition() const;
};

// CC requires bool array to reset.
struct GameBoard
{
	bool field[400];
};

enum class ControllerAction
{
	None,
	Left,
	Right,
	Down,
	RotateClockwise,
	RotateCounterClockwise,
	FastLeft,
	FastRight,
	FastDrop,
	HardDrop,
};

const wchar_t* GetControllerActionDescription(ControllerAction act);

struct ControllerHint
{
	std::vector<ControllerAction> actions;
	int cost;
};

struct SRSInfo
{
	int state;
	int index;
	int x, y;
};

class TetrisGame
{
public:
	TetrisGame();
	void PushNextPiece(MinoType t);
	void PushBackCurrentPiece();
	const Tetrimino& GetCurrentPiece() const;
	MinoType GetNextPiece(int index) const;
	MinoType GetHoldPiece() const;
	MinoType PopNextPiece();
	MinoType HoldCurrentPiece();
	bool MoveLeft();
	bool MoveRight();
	bool MoveDown();
	bool RotateClockwise();
	bool RotateCounterClockwise();

	void LockCurrentPiece();
	Tetrimino GetGhost() const;
	bool SpawnCurrentPiece();
	void ResetCurrentPiece();

	bool ClearLines();
	int GetHighestLine() const;
	void AddGarbage(int lines, float holeRepeat);

	void SetGravity(float gravity);
	void SetColor(int x, int y, MinoType color);
	MinoType GetColor(int x, int y) const;
	int RemainingNext() const;
	int GetPieceIndex() const;

	void GetBoard(GameBoard& b) const;
	void GetSRSInfo(SRSInfo& cw, SRSInfo& ccw);

	ControllerHint FindPath(const unsigned char expectedX[4], const unsigned char expectedY[4]);
private:
	int FindPathIndex() const;
	bool FastDrop();
	bool FastLeft();
	bool FastRight();
	bool Is20G() const;
	SRSInfo TryRotateClockwise();
	SRSInfo TryRotateCounterClockwise();

	unsigned int board[VISIBLE_LINES * 2]; // One uint per line, one bit per cell. Bottom up.
	MinoType boardColor[VISIBLE_LINES * 2][BOARD_WIDTH]; // Color of each cell
	Tetrimino current;

	MinoType hold;
	int pieceIndex;
	std::deque<MinoType> next;
	std::mutex m;
	float gravity;
};

}