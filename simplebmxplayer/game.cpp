#include "game.h"
#include "gameplay.h"
#include "luamanager.h"
#include "util.h"
#include "globalresources.h"
#include "font.h"


namespace Game {
	// game status
	GameSetting setting;
	bool bRunning = true;		// is game running?

	// bms info
	std::string bmspath;

	// SDL
	SDL_Window* WINDOW = NULL;
	SDL_Renderer* RENDERER = NULL;

	// drawing texture/fonts
	// (it'll be automatically released, it's pool object - managed by pool)
	Font *font;

	// FPS
	Timer fpstimer;
	int fps = 0;

	void Game::Parameter::help() {
		wprintf(L"SimpleBmxPlayer\n================\n-- How to use -- \n\n"
			L"argument: (bmx file) (options ...)\n"
			L"options:"
			L"-noimage: don't load image files\n"
			L"-ms: start from n-th measure\n"
			L"-repeat: repeat bms for n-times\n"
			L"keys:\n"
			L"default key config is -\n(1P) LS Z S X D C F V (2P) M K , L . ; / RS\nyou can change it by changing preset files.\n"
			L"Press F5 to reload BMS file only.\n"
			L"Press F6 to reload BMS Resource file only.\n"
			L"Press - to move -5 measures.\n"
			L"Press + to move +5 measures.\n"
			L"Press Up/Down to change speed.\n"
			L"Press Right/Left to change lane.\n"
			L"Press Shift+Right/Left to change lift.\n");
	}

	bool Game::Parameter::parse(int argc, _TCHAR **argv) {
		char *buf = new char[10240];
		if (argc <= 1)
			return false;

		ENCODING::wchar_to_utf8(argv[1], buf, 10240);	bmspath = buf;

		delete buf;
		return true;
	}

	/* 
	 * registering basic lua function start 
	 * TODO: generating rendering object (after we're done enough)
	 */
	namespace {
		int InitTimer(Lua* l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			TIMERPOOL->Set(key, false);
			// no return value
			return 0;
		}

		int SetTimer(Lua* l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			TIMERPOOL->Set(key);
			//lua_pushinteger(l, a + 1);
			// no return value
			return 0;
		}

		int SetInt(Lua* l) {
			int val = (int)lua_tointeger(l, -1);
			lua_pop(l, 1);
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			INTPOOL->Set(key, val);
			return 0;	// no return value
		}

		int SetFloat(Lua* l) {
			int val = (int)lua_tointeger(l, -1);
			lua_pop(l, 1);
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			INTPOOL->Set(key, val);
			return 0;	// no return value
		}

		int SetString(Lua* l) {
			int val = (int)lua_tointeger(l, -1);
			lua_pop(l, 1);
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			INTPOOL->Set(key, val);
			return 0;	// no return value
		}

		int IsTimer(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			Timer* t = TIMERPOOL->Get(key);
			if (t && t->IsStarted())
				lua_pushboolean(l, 1);
			else
				lua_pushboolean(l, 0);
			return 1;
		}

		int GetTime(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			Timer *t = TIMERPOOL->Get(key);
			if (t)
				lua_pushinteger(l, t->GetTick());
			else
				lua_pushinteger(l, 0);
			return 1;
		}

		int GetInt(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			int *v = INTPOOL->Get(key);
			if (v)
				lua_pushinteger(l, *v);
			else
				lua_pushinteger(l, 0);
			return 1;
		}

		int GetFloat(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			double *d = DOUBLEPOOL->Get(key);
			if (d)
				lua_pushnumber(l, *d);
			else
				lua_pushnumber(l, 0);
			return 1;
		}

		int GetString(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			RString *s = STRPOOL->Get(key);
			if (s)
				lua_pushstring(l, *s);
			else
				lua_pushstring(l, "");
			return 1;
		}

		void RegisterBasicLuaFunction() {
			Lua *l;
			l = LUA->Get();
			lua_register(l, "SetTimer", SetTimer);
			lua_register(l, "SetSwitch", SetTimer);
			lua_register(l, "InitTimer", InitTimer);
			lua_register(l, "InitSwitch", InitTimer);
			lua_register(l, "SetInt", SetInt);
			lua_register(l, "SetFloat", SetFloat);
			lua_register(l, "SetString", SetString);
			lua_register(l, "IsTimer", IsTimer);
			lua_register(l, "IsSwitch", IsTimer);
			lua_register(l, "GetTime", GetTime);
			lua_register(l, "GetInt", GetInt);
			lua_register(l, "GetFloat", GetFloat);
			lua_register(l, "GetString", GetString);
			LUA->Release(l);
		}
	}

	bool Init() {
		/*
		 * Basic instances initalization
		 */
		GameTimer::Tick();
		PoolHelper::InitalizeAll();
		LUA = new LuaManager();
		RegisterBasicLuaFunction();

		/*
		 * Load basic setting file ...
		 */
		if (!setting.LoadSetting("../setting.xml")) {
			wprintf(L"Cannot load settings files... Use Default settings...\n");
			if (!setting.DefaultSetting()) {
				wprintf(L"CANNOT load default settings. program exit.\n");
				return -1;
			}
		}

		/*
		 * Game engine initalize
		 */
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			wprintf(L"Failed to SDL_Init() ...\n");
			return -1;
		}
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {	// no lag, no sound latency
			wprintf(L"Failed to Open Audio ...\n");
			return -1;
		}
		Mix_AllocateChannels(1296);
		WINDOW = SDL_CreateWindow("SimpleBmxPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			setting.mEngine.mWidth, setting.mEngine.mHeight, SDL_WINDOW_SHOWN);
		if (!WINDOW) {
			wprintf(L"Failed to create window\n");
			return -1;
		}
		RENDERER = SDL_CreateRenderer(WINDOW, -1, SDL_RENDERER_ACCELERATED);// | SDL_RENDERER_PRESENTVSYNC);
		if (!RENDERER) {
			wprintf(L"Failed to create Renderer\n");
			return -1;
		}

		/*
		 * Scene instance initalization
		 * (MUST after graphic initalization finished)
		 */
		GamePlay::Init();

		/*
		 * prepare game basic resource
		 */
		font = FONTPOOL->LoadTTFFont("_system", "../skin/lazy.ttf", 28, FC_MakeColor(120, 120, 120, 255));

		/*
		 * prepare GamePlay
		 */
		STRPOOL->Set("PlaySkinpath", "../skin/Wisp_HD/play/HDPLAY_W.lr2skin");
		STRPOOL->Set("Bmspath", bmspath);

		return true;
	}

	void Start() {

		/*
		 * FPS timer start & initalize
		 */
		fpstimer.Start();
		fps = 0;

		/*
		 * GamePlay Start
		 * - We don't need select screen, only play screen.
		 */
		GamePlay::Start();
	}

	void Render_FPS() {
		// calculate FPS per 1 sec
		float avgfps = fps / (fpstimer.GetTick() / 1000.0f);
		fps++;
		// print FPS
		font->Render(ssprintf("%.2f Frame", avgfps), 0, 0);
		if (fpstimer.GetTick() > 1000) {
			wprintf(L"%.2f\n", avgfps);
			fpstimer.Stop();
			fpstimer.Start();
			fps = 0;
		}
	}

	void Render() {
		// nothing to do just render GamePlay ...
		GamePlay::Render();
	}

	void MainLoop() {
		while (bRunning) {
			/*
			 * Ticking
			 */
			GameTimer::Tick();

			/*
			 * Keybd, mouse event
			 * TODO: support event handler
			 */
			SDL_Event e;
			if (SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT) {
					bRunning = false;
				}
				else if (e.type == SDL_KEYDOWN) {
					switch (e.key.keysym.sym) {
					case SDLK_ESCAPE:
						bRunning = false;
						break;
					default:
						GamePlay::KeyPress(e.key.keysym.sym);
					}
				}
				else if (e.type == SDL_KEYUP) {
					switch (e.key.keysym.sym) {
					default:
						GamePlay::KeyUp(e.key.keysym.sym);
					}
					//TIMERPOOL->Reset("OnScene");
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {

				}
				else if (e.type == SDL_MOUSEBUTTONUP) {

				}
				else if (e.type == SDL_MOUSEMOTION) {

				}
				else if (e.type == SDL_MOUSEWHEEL) {

				}
			}

			/*
			 * Graphic rendering
			 * - skin and movie(Image::Refresh) part are departed from main thread
			 *   so keypress will be out of lag
			 *   (TODO)
			 */
			SDL_RenderClear(RENDERER);
			Render();
			Render_FPS();
			SDL_RenderPresent(RENDERER);
		}
	}

	void Game::Release() {
		// stop Bms loading && release Bms
		BmsHelper::ReleaseAll();

		// other scenes
		GamePlay::Release();

		// release basic instances
		PoolHelper::ReleaseAll();
		delete LUA;

		// finally, game engine (audio/renderer) release
		Mix_CloseAudio();
		SDL_DestroyRenderer(RENDERER);
		SDL_DestroyWindow(WINDOW);
	}
}