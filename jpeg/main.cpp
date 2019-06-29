//
#include <iostream>
#include <cassert>
#include <cstdio>
#include <vector>

//
#include "Engine.h"

// SDL
#include <SDL.h>
#include <SDL_image.h>

// Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Pixels dimension
const int NUM_PIXEL_X = 802;
const int NUM_PIXEL_Y = 602;

// include the Direct3D Library file
#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2main.lib")
#pragma comment (lib, "SDL2test.lib")
#pragma comment (lib, "SDL2_image.lib")

using namespace std;



int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("SDL initialization failed. SDL Error: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);



	// SDL event handler
	SDL_Event e;
	bool bStatusQuit = false;
	bool bStatusPainting = false;
	int iPosX, iPosY;



	// Image data
	Canvas* canvas = nullptr;
	canvas = new Canvas;
	if (!canvas)
	{
		cout << "Canvas initialization failed" << endl;
		bStatusQuit = true;
	}
	else
	{
		canvas->Init(NUM_PIXEL_X, NUM_PIXEL_Y);
		canvas->SetAllPixels({ 0, 0, 0, SDL_ALPHA_OPAQUE });
	}



	// PNG image handler
	int iImageFlags = IMG_INIT_PNG;

	if (!(IMG_Init(iImageFlags) & iImageFlags))
	{
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return 1;
	}



	// The window to be rendered
	SDL_Window* pWnd = NULL;

	// Set up Screen
	pWnd = SDL_CreateWindow(
		"JPEG Viewer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN);

	if (pWnd == NULL)
	{
		printf("SDL window could not be created. SDL Error: %s\n", SDL_GetError());
		return 1;
	}



	// The surface contained by the window
	SDL_Surface* pScreen = NULL;

	// Get window surface
	pScreen = SDL_GetWindowSurface(pWnd);



	// The window renderer
	SDL_Renderer* pRenderer = NULL;

	// Create renderer for window
	pRenderer = SDL_CreateRenderer(pWnd, -1, SDL_RENDERER_ACCELERATED);
	if (pRenderer == NULL)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_RendererInfo rnInfo;
	SDL_GetRendererInfo(pRenderer, &rnInfo);
	cout << "Renderer name: " << rnInfo.name << endl;
	cout << "Texture formats: " << endl;
	for (unsigned int i = 0; i < rnInfo.num_texture_formats; ++i)
		cout << SDL_GetPixelFormatName(rnInfo.texture_formats[i]) << endl;

	// Initialize renderer color
	SDL_SetRenderDrawColor(pRenderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);



	// Streaming enabled texture for main canvas
	SDL_Texture* pTexPrimeCanvas{ NULL };
	pTexPrimeCanvas = SDL_CreateTexture(
		pRenderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING,
		canvas->width(),
		canvas->height());



	// Fill the surface
	SDL_FillRect(pScreen, NULL, SDL_MapRGB(pScreen->format, 0x00, 0x00, 0x00));

	// Update the surface
	SDL_UpdateWindowSurface(pWnd);



#define DEBUG_WRITE_TO_JPEG
#define DEBUG_READ_FROM_JPEG

#ifdef DEBUG_WRITE_TO_JPEG
	const int w = canvas->width(), h = canvas->height();
	float cx = w / 2, cy = h / 2;
	float am = 256.f * cx, sigma = w / 2.f;
	auto gauss = [](int x, int y, const vector<float>& param) {
		float dx = x - param[2], dy = y - param[3];
		float d2 = dx * dx + dy * dy;
		float am = param[0], sigma = param[1];
		return am * expf(-d2 / (sigma * sigma * 2.f)) / (sigma * sqrtf(2.f));
	};
	canvas->SetAllPixels({ gauss, gauss, gauss, gauss },
		{ {am, sigma, cx, 0}, {am, sigma, 0, cy}, {am, sigma, cx, cy}, {}, },
		0b1110);

	canvas->SaveAsJPEG(string("TEST_JPEG.txt"), 5);
	//bStatusQuit = true;
#endif

#ifdef DEBUG_READ_FROM_JPEG
	canvas->ReadAsJPEG(string("TEST_JPEG.txt"));
#endif



	SDL_RenderClear(pRenderer);
	SDL_UpdateTexture(pTexPrimeCanvas, NULL, canvas->pixels(), canvas->width() * 4);
	SDL_RenderCopy(pRenderer, pTexPrimeCanvas, NULL, NULL);
	SDL_RenderPresent(pRenderer);



	// main loop
	while (!bStatusQuit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				bStatusQuit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				bStatusPainting = true;
			case SDL_MOUSEMOTION:
				if (bStatusPainting)
				{
					iPosX = (int) (e.button.x / (double)SCREEN_WIDTH * canvas->width());
					iPosY = (int) (e.button.y / (double)SCREEN_HEIGHT* canvas->height());
					canvas->EditPixel(iPosX, iPosY, { 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE }, 10);
					SDL_RenderClear(pRenderer);
					SDL_UpdateTexture(pTexPrimeCanvas, NULL, canvas->pixels(), canvas->width() * 4);
					SDL_RenderCopy(pRenderer, pTexPrimeCanvas, NULL, NULL);
					SDL_RenderPresent(pRenderer);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				bStatusPainting = false;
				break;
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym)
				{
				case SDLK_s:
					canvas->SaveAsJPEG(string("TEST_JPEG.txt"), 5);
					break;
				case SDLK_ESCAPE:
					bStatusQuit = true;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
	}



	// Destroy window
	SDL_DestroyTexture(pTexPrimeCanvas);
	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(pWnd);

	// Destroy data
	canvas->Free();
	if(canvas) delete canvas;

	// Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();

	return 0;
}
