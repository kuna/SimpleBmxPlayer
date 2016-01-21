#include "SDL/SDL.h"
#include "SDL/SDL_FontCache.h"	// extract it to a class
#include "SDL/SDL_timer.h"
#include "image.h"
#include "audio.h"
#include "timer.h"
#include "bmsresource.h"
#include "player.h"
#include "skin.h"
#include "gamesetting.h"
#include <tchar.h>

namespace Game {
	namespace Parameter {
		void help();
		bool parse(int argc, _TCHAR **argv);
	}

	bool Init();
	void Start();
	void MainLoop();
	void Release();

	// get/set
	extern SDL_Renderer* RENDERER;
	extern SDL_Window* WINDOW;
}