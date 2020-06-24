#include <windows.h>
#include "input.h"

#pragma comment (lib, "SDL2.lib")
#include <SDL.h>
#include <SDL_gamecontroller.h>

using namespace Maneru;

static void DoNothing() {}

struct AutoButton
{
	AutoButton();
	typedef std::function<void(void)> ButtonEvent;
	LARGE_INTEGER last;
	int down;
	float repeatDelay;
	float repeatRate;
	bool repeat;
	void Set(WORD value);
	void SetRepeat(double start, double rate);

	ButtonEvent Fire;
};

AutoButton::AutoButton() : down(0), repeatDelay(0), repeatRate(0), repeat(false), Fire(DoNothing)
{
	last.QuadPart = 0;
}

void AutoButton::Set(WORD value)
{
	if (value != 0)
	{
		if (down == 0)
		{
			down = 1;
			Fire();
			QueryPerformanceCounter(&last);
			repeat = false;
		}
		else
		{
			if (repeatDelay > 0)
			{
				LARGE_INTEGER now, freq;
				QueryPerformanceCounter(&now);
				QueryPerformanceFrequency(&freq);
				double diff = now.QuadPart - last.QuadPart;
				if (repeat)
				{
					if (diff > freq.QuadPart * repeatRate)
					{
						Fire();
						last.QuadPart = now.QuadPart;
					}
				}
				else
				{
					if (diff > freq.QuadPart * repeatDelay)
					{
						Fire();
						last.QuadPart = now.QuadPart;
						repeat = true;
					}
				}
			}
		}
	}
	else
	{
		if (down == 1)
		{
			down = 0;
			last.QuadPart = 0;
			repeat = false;
		}
	}
}

void AutoButton::SetRepeat(double start, double rate)
{
	repeatDelay = start;
	repeatRate = rate;
}

struct SDLButtonBinding
{
	std::string name;
	SDL_GameControllerButton sdl;
	AutoButton btn;
};

const int NUM_BUTTONS = 15;

class Controller : public ControllerInterface
{
public:
	Controller(SDL_GameController* ctrl) : ctrl(ctrl), buttons
	{
		{"up", SDL_CONTROLLER_BUTTON_DPAD_UP},
		{"down", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
		{"left", SDL_CONTROLLER_BUTTON_DPAD_LEFT},
		{"right", SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
		{"a", SDL_CONTROLLER_BUTTON_A},
		{"b", SDL_CONTROLLER_BUTTON_B},
		{"x", SDL_CONTROLLER_BUTTON_X},
		{"y", SDL_CONTROLLER_BUTTON_Y},
		{"lb", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
		{"rb", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
		{"ls", SDL_CONTROLLER_BUTTON_LEFTSTICK},
		{"rs", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
		{"start", SDL_CONTROLLER_BUTTON_START},
		{"back", SDL_CONTROLLER_BUTTON_BACK},
		{"guide", SDL_CONTROLLER_BUTTON_GUIDE},
	}
	{
	}
	virtual void Refresh()
	{
		SDL_GameControllerUpdate();
		for (SDLButtonBinding& bind : buttons)
		{
			bind.btn.Set(SDL_GameControllerGetButton(ctrl, bind.sdl));
		}
	}
	// Inherited via ControllerInterface
	virtual void SetButtonFunction(const char* name, std::function<void(void)>& f, float repeatDelay, float repeatRate)
	{
		for (SDLButtonBinding& bind : buttons)
		{
			if (bind.name == name)
			{
				bind.btn.Fire = f;
				bind.btn.repeatDelay = repeatDelay;
				bind.btn.repeatRate = repeatRate;
				return;
			}
		}
	}
private:
	SDLButtonBinding buttons[NUM_BUTTONS];
	SDL_GameController* ctrl;
};

ControllerInterface* Maneru::OpenController(int index)
{
	SDL_GameController *ctrl = SDL_GameControllerOpen(index);
	if (ctrl == nullptr) return nullptr;
	return new Controller(ctrl);
}

ControllerList Maneru::GetControllerList()
{
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	ControllerList controllers;
	int n = SDL_NumJoysticks();
	for (int i = 0; i < n; i++)
	{
		if (SDL_IsGameController(i))
		{
			const char* name = SDL_GameControllerNameForIndex(i);
			controllers[name] = i;
		}
	}
	return controllers;
}
