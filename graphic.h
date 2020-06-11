#pragma once

#include <windows.h>
#include "game.h"

namespace Maneru
{

class GraphicEngineInterface
{
public:
	virtual void Shutdown() = 0;
	virtual void DrawScreen(const TetrisGame& game) = 0;
	virtual void DrawHint(unsigned char x[4], unsigned char y[4], MinoType type, float alpha) = 0;
	virtual void DrawHoldHint(float alpha) = 0;
	virtual void DrawStats(LPCWSTR *lines, int num) = 0;
	virtual void StartDraw() = 0;
	virtual void FinishDraw() = 0;
};

GraphicEngineInterface* InitGraphicEngine(HWND hwnd, float magnify, int fps);

}
