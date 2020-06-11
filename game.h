#pragma once
#include <mutex>
#include <deque>

namespace Maneru
{
enum MinoType
{
	PieceI,
	PieceT,
	PieceO,
	PieceS,
	PieceZ,
	PieceL,
	PieceJ,
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

class TetrisGame
{
public:
	TetrisGame();
	void PushNextPiece(MinoType t);
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

	bool ClearLines();
	void AddGarbage(int lines, float holeRepeat);

	void SetColor(int x, int y, MinoType color);
	MinoType GetColor(int x, int y) const;
	int RemainingNext() const;
	int GetPieceIndex() const;

	GameBoard GetBoard() const;
private:
	unsigned int board[VISIBLE_LINES * 2]; // One uint per line, one bit per cell. Bottom up.
	MinoType boardColor[VISIBLE_LINES * 2][BOARD_WIDTH]; // Color of each cell
	Tetrimino current;

	MinoType hold;
	int pieceIndex;
	std::deque<MinoType> next;
	std::mutex m;
};

}