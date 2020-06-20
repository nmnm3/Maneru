#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include "graphic.h"
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")
#pragma comment (lib, "d3d11.lib")


using namespace Maneru;
using D2D1::ColorF;

const int MINO_SIZE_BASE = 20;
const int BOARD_LEFT_BASE = 150;
const int TEXT_LINE_SIZE_BASE = 50;
const int TEXT_FONT_SIZE_BASE = 18;

POINT PieceRect[7][4] =
{
	{{1, 2}, {2, 2}, {3, 2}, {4, 2}}, //I
	{{1, 1}, {1, 2}, {2, 1}, {2, 2}}, //O
	{{1, 2}, {2, 2}, {3, 2}, {2, 1}}, //T
	{{1, 2}, {2, 2}, {3, 2}, {3, 1}}, //L
	{{1, 1}, {1, 2}, {2, 2}, {3, 2}}, //J
	{{1, 2}, {2, 2}, {2, 1}, {3, 1}}, //S
	{{1, 1}, {2, 1}, {2, 2}, {3, 2}}, //Z

};


ColorF brushColors[] =
{
	0xE6FE, //I
	0xFFDE00, //O
	0xB802FD, //T
	0xFF7308, //L
	0x1801FF, //J
	0x66FD00, //S
	0xFD103C, //Z
	ColorF::Gray, //Garbage
	ColorF::Black, //None
	ColorF::White,
};

class GraphicEngine : public GraphicEngineInterface
{
public:
	GraphicEngine(HWND hwnd, float magnify, int fps) :
		hwnd(hwnd),
		refreshFrequency{},
		d2dev{},
		d2dctx{},
		swapchain{},
		renderTarget{},
		textFormat{},
		textBrush{},
		minoBrush{},
		fps(fps)
	{
		QueryPerformanceFrequency(&refreshFrequency);
		refreshFrequency.QuadPart /= fps;
		MINO_SIZE = MINO_SIZE_BASE * magnify;
		BOARD_LEFT = BOARD_LEFT_BASE * magnify;
		TEXT_LINE_SIZE = TEXT_LINE_SIZE_BASE * magnify;
		TEXT_FONT_SIZE = TEXT_FONT_SIZE_BASE * magnify;
	}
	virtual void StartDraw()
	{
		renderTarget->BeginDraw();
		renderTarget->Clear(ColorF(ColorF::White));
	}

	virtual void FinishDraw()
	{
		renderTarget->EndDraw();

		static LARGE_INTEGER target, current;
		if (target.QuadPart > 0)
		{
			QueryPerformanceCounter(&current);
			while (current.QuadPart < target.QuadPart)
			{
				Sleep(1);
				QueryPerformanceCounter(&current);
			}
			while (current.QuadPart > target.QuadPart)
				target.QuadPart += refreshFrequency.QuadPart;
		}
		else
		{
			QueryPerformanceCounter(&target);
		}

		swapchain->Present(0, 0);
	}
	virtual void DrawBoard(const TetrisGame& game)
	{
		D2D1_RECT_F rect;
		rect.top = (VISIBLE_LINES - 1) * MINO_SIZE;
		rect.bottom = rect.top + MINO_SIZE;

		for (int i = 0; i < VISIBLE_LINES; i++)
		{
			rect.left = BOARD_LEFT;
			rect.right = BOARD_LEFT + MINO_SIZE;
			for (int j = 0; j < 10; j++)
			{
				MinoType t = game.GetColor(i, j);
				ID2D1SolidColorBrush* brush = minoBrush[t];

				renderTarget->FillRectangle(rect, brush);
				rect.left += MINO_SIZE;
				rect.right += MINO_SIZE;
			}
			rect.top -= MINO_SIZE;
			rect.bottom -= MINO_SIZE;
		}
	}

	virtual void DrawCurrentPiece(const Tetrimino& piece)
	{
		if (piece.type == PieceNone) return;

		D2D1_RECT_F rect;
		const BlockPosition& pos = piece.GetPosition();
		for (int y = 0; y < 4; y++)
		{
			unsigned int mask = pos[y];
			if (mask == 0) continue;
			for (int x = 0; x < 4; x++)
			{
				if (mask & (1 << x))
				{
					rect.left = BOARD_LEFT + (piece.px + x) * MINO_SIZE;
					rect.right = rect.left + MINO_SIZE;
					rect.bottom = (VISIBLE_LINES - piece.py - y) * MINO_SIZE;
					rect.top = rect.bottom - MINO_SIZE;
					renderTarget->FillRectangle(rect, minoBrush[piece.type]);
				}
			}
		}
	}

	virtual void DrawPieceView(MinoType piece, MinoType color, int x, int y)
	{
		D2D1_RECT_F rect;

		rect.left = x;
		rect.top = y;
		rect.right = x + MINO_SIZE * 5;
		rect.bottom = y + MINO_SIZE * 5;

		renderTarget->FillRectangle(rect, minoBrush[PieceNone]);
		if (piece != PieceNone)
		{
			switch (piece)
			{
			case PieceI:
				x -= MINO_SIZE / 2; break;
			case PieceO:
				x += MINO_SIZE / 2;
				y += MINO_SIZE / 2;
				break;
			default:
				y += MINO_SIZE / 2; break;
			}
			for (int i = 0; i < 4; i++)
			{
				D2D1_RECT_F r;
				POINT p = PieceRect[piece][i];
				r.left = x + p.x * MINO_SIZE;
				r.top = y + p.y * MINO_SIZE;
				r.right = r.left + MINO_SIZE;
				r.bottom = r.top + MINO_SIZE;
				renderTarget->FillRectangle(r, minoBrush[color]);
			}
		}
	}

	virtual void DrawHold(MinoType hold, bool disabled)
	{
		MinoType color = disabled ? PieceGarbage : hold;
		DrawPieceView(hold, color, 0, 0);
	}
	virtual void DrawNextPreview(const TetrisGame& game)
	{
		int r = game.RemainingNext();
		int pieceLeft1 = BOARD_LEFT + MINO_SIZE * 12;
		int pieceLeft2 = pieceLeft1 + MINO_SIZE * 5;
		for (int i = 0; i < 4; i++)
		{
			if (i < r)
			{
				MinoType t = game.GetNextPiece(i);
				DrawPieceView(t, t, pieceLeft1, i * MINO_SIZE * 5);
			}
			if (i + 4 < r)
			{
				MinoType t = game.GetNextPiece(i + 4);
				DrawPieceView(t, t, pieceLeft2, i * MINO_SIZE * 5);
			}
				
		}
	}
	virtual void DrawHoldHint(float alpha)
	{
		ID2D1Layer *layer;
		renderTarget->CreateLayer(&layer);
		D2D1_LAYER_PARAMETERS params =
			D2D1::LayerParameters(
				D2D1::InfiniteRect(),
				NULL,
				D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::IdentityMatrix(),
				alpha,
				minoBrush[PieceGarbage],
				D2D1_LAYER_OPTIONS_NONE);

		renderTarget->PushLayer(params, layer);
		D2D1_RECT_F rect;
		rect.left = 0;
		rect.right = MINO_SIZE * 5;
		rect.top = MINO_SIZE * 4;
		rect.bottom = rect.top + MINO_SIZE;

		renderTarget->FillRectangle(rect, minoBrush[PieceHint]);
		renderTarget->PopLayer();
		layer->Release();
	}

	virtual void DrawStats(LPCWSTR *lines, int num)
	{
		D2D1_RECT_F r;
		int top = MINO_SIZE * 6;
		r.left = 0;
		r.right = BOARD_LEFT;
		for (int i = 0; i < num; i++)
		{
			r.top = top + i * TEXT_FONT_SIZE;
			r.bottom = r.top + TEXT_FONT_SIZE;
			renderTarget->DrawTextW(lines[i], wcslen(lines[i]), textFormat, r, textBrush);
		}
	}

	virtual void DrawHint(unsigned char x[4], unsigned char y[4], MinoType type, float alpha)
	{
		ID2D1Layer *layer;
		renderTarget->CreateLayer(&layer);
		D2D1_LAYER_PARAMETERS params =
			D2D1::LayerParameters(
				D2D1::InfiniteRect(),
				NULL,
				D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::IdentityMatrix(),
				alpha,
				minoBrush[type],
				D2D1_LAYER_OPTIONS_NONE);

		renderTarget->PushLayer(params, layer);
		float stroke = MINO_SIZE / 10.0;
		for (int i = 0; i < 4; i++)
		{
			unsigned char xx = x[i];
			unsigned char yy = y[i];
			D2D1_RECT_F rect;

			rect.left = BOARD_LEFT + xx * MINO_SIZE + stroke;
			rect.right = rect.left + MINO_SIZE - stroke;
			rect.bottom = (VISIBLE_LINES - yy) * MINO_SIZE - stroke;
			rect.top = rect.bottom - MINO_SIZE + stroke;
			renderTarget->DrawRectangle(rect, minoBrush[type], stroke);
		}
		renderTarget->PopLayer();
		layer->Release();
	}

	virtual void Shutdown()
	{
		// TODO: do something...
	}

	bool Init()
	{
		HRESULT hr;
		if (!InitDevice3D(hwnd))
		{
			return false;
		}

		hr = renderTarget->CreateSolidColorBrush(ColorF(ColorF::Black), &textBrush);
		if (FAILED(hr)) return false;

		for (int i = 0; i < ARRAYSIZE(brushColors); i++)
		{
			hr = renderTarget->CreateSolidColorBrush(brushColors[i], minoBrush + i); //I
			if (FAILED(hr)) return false;
		}

		IDWriteFactory* pDWFactory = nullptr;
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(pDWFactory),
			(IUnknown **)&pDWFactory
		);
		if (FAILED(hr)) return false;

		hr = pDWFactory->CreateTextFormat(L"MS PGOTHIC", 0,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			TEXT_FONT_SIZE,
			L"",
			&textFormat);
		if (FAILED(hr)) return false;
		return true;
	}

private:
	HWND hwnd;
	LARGE_INTEGER refreshFrequency;
	ID2D1Device* d2dev;
	ID2D1DeviceContext* d2dctx;
	IDXGISwapChain* swapchain;
	ID2D1RenderTarget* renderTarget;
	IDWriteTextFormat* textFormat;
	ID2D1SolidColorBrush* textBrush;

	ID2D1SolidColorBrush* minoBrush[ARRAYSIZE(brushColors)];
	int MINO_SIZE;
	int BOARD_LEFT;
	int TEXT_LINE_SIZE;
	int TEXT_FONT_SIZE;
	int fps;

	bool InitDevice3D(HWND hwnd)
	{
		HRESULT hr;
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		D3D_FEATURE_LEVEL level;

		ID3D11Device* dev = 0;
		ID3D11DeviceContext* dctx = 0;

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 1;
		desc.BufferDesc.RefreshRate.Denominator = fps;
		desc.OutputWindow = hwnd;
		desc.Windowed = true;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		desc.Flags = 0;

		hr = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &desc, &swapchain, &dev, &level, &dctx);
		if (FAILED(hr)) return false;

		IDXGIDevice* dxgi;
		hr = dev->QueryInterface(&dxgi);
		if (FAILED(hr)) return false;

		ID2D1Factory1* factory = 0;
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&factory
		);
		if (FAILED(hr)) return false;

		ID2D1Device* d2dev;
		ID2D1DeviceContext* d2dctx;
		hr = factory->CreateDevice(dxgi, &d2dev);
		if (FAILED(hr)) return false;
		hr = d2dev->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dctx);
		if (FAILED(hr)) return false;

		D2D1_BITMAP_PROPERTIES1 prop = {};
		prop.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
		prop.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
		prop.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		d2dctx->GetDpi(&prop.dpiX, &prop.dpiY);

		IDXGISurface *backsurface;
		hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&backsurface));
		if (FAILED(hr)) return false;

		ID2D1Bitmap1 *backbmp;
		hr = d2dctx->CreateBitmapFromDxgiSurface(backsurface, &prop, &backbmp);
		if (FAILED(hr)) return false;
		d2dctx->SetTarget(backbmp);
		renderTarget = d2dctx;
		return true;
	}
};

GraphicEngineInterface* Maneru::InitGraphicEngine(HWND hwnd, float magnify, int fps)
{
	GraphicEngine* engine = new GraphicEngine(hwnd, magnify, fps);
	if (engine->Init())
		return engine;
	else
	{
		delete engine;
		return nullptr;
	}
}