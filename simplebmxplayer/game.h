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
	struct GameSetting {
		// basic information
		int width, height;
		bool vsync;
		bool allowaddon;
		int volume;
		bool tutorial;

		// skin (not skin option)
		RString skin_main;
		RString skin_user;
		RString skin_select;
		RString skin_decide;
		RString skin_play_5key;
		RString skin_play_7key;
		RString skin_play_9key;
		RString skin_play_10key;
		RString skin_play_14key;
		RString skin_result;

		// user select
		RString username;

		// song select
		int keymode;

		// game play
		// - NOPE

		// result screen
		// - NOPE
	};

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