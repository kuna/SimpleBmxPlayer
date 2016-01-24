#include "game.h"
#include "gameplay.h"

#include "luamanager.h"
#include "util.h"
#include "globalresources.h"
#include "font.h"
#include "file.h"
#include "version.h"
#include "tinyxml2.h"
#include "bmsresource.h"
#include "playerinfo.h"
#include "player.h"

using namespace tinyxml2;

namespace Game {
	/*
	 * variables
	 */
	// game status
	GameSetting		SETTING;
	SceneBasic*		SCENE = NULL;
	bool			bRunning = false;	// is game running?

	// SDL
	SDL_Window*		WINDOW = NULL;
	SDL_Renderer*	RENDERER = NULL;

	// FPS
	Timer			fpstimer;
	int				fps = 0;
	Font			*font;				// system font
	bool			showfps = true;

	// basic
	Timer			*oninputstart;
	Timer			*onscene;

	// some macros about scene
	void InitalizeScene(SceneBasic* s) { s->Initialize(); }
	void ReleaseScene(SceneBasic* s) { s->Release(); }
	void EndScene(SceneBasic *s) { s->End(); }

	void StartScene(SceneBasic *s) {
		/*
		* inputstart/scene timer is a little different;
		* sometimes input blocking is necessary during scene rendering.
		*/
		oninputstart->Start();
		onscene->Start();
		s->Start();
	}

	void ChangeScene(SceneBasic *s) {
		if (SCENE != s)
			StartScene(s);
		SCENE = s;
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

	bool Initialize() {
		/*
		 * Load basic setting file ...
		 */
		if (!GameSettingHelper::LoadSetting(SETTING)) {
			wprintf(L"Cannot load settings files... Use Default settings...\n");
			GameSettingHelper::DefaultSetting(SETTING);
		}

		/*
		 * Load Player information ...
		 */
		if (!PlayerInfoHelper::LoadPlayerInfo(PLAYERINFO[0], SETTING.username))
			PlayerInfoHelper::DefaultPlayerInfo(PLAYERINFO[0]);

		/*
		 * Basic instances initalization
		 */
		GameTimer::Tick();
		LUA = new LuaManager();
		RegisterBasicLuaFunction();

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
		int flag = SDL_WINDOW_SHOWN;
		if (SETTING.resizable)
			flag |= SDL_WINDOW_RESIZABLE;
		if (SETTING.vsync)
			flag |= SDL_RENDERER_PRESENTVSYNC;
		WINDOW = SDL_CreateWindow(PROGRAMNAME " - " PROGRAMDATE "(" PROGRAMCOMMIT ")", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			SETTING.width, SETTING.height, SDL_WINDOW_SHOWN);
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
		GamePlay::SCENE = new GamePlay::ScenePlay();
		InitalizeScene(GamePlay::SCENE);

		/*
		 * prepare game basic resource
		 */
		oninputstart = TIMERPOOL->Get("OnInputStart");
		onscene = TIMERPOOL->Get("OnScene");
		font = FONTPOOL->LoadTTFFont("_system", "../system/resource/NanumGothic.ttf", 28, FC_MakeColor(120, 120, 120, 255));

		/*
		 * FPS timer start
		 */
		fpstimer.Start();
		fps = 0;

		bRunning = true;
		return true;
	}

	// private?
	void Render_FPS() {
		// calculate FPS per 1 sec
		float avgfps = fps / (fpstimer.GetTick() / 1000.0f);
		fps++;
		// print FPS
		font->Render(ssprintf("%.2f Frame", avgfps), 10, 10);
		if (fpstimer.GetTick() > 1000) {
			wprintf(L"%.2f\n", avgfps);
			fpstimer.Stop();
			fpstimer.Start();
			fps = 0;
		}
	}

	void MainLoop() {
		while (bRunning) {
			if (!SCENE) continue;

			/*
			 * Ticking
			 */
			GameTimer::Tick();

			/*
			 * Keybd, mouse event
			 */
			SDL_Event e;
			if (oninputstart->GetTick() > 1000 && SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT) {
					End();
				}
				else if (e.type == SDL_KEYDOWN) {
					switch (e.key.keysym.sym) {
					case SDLK_ESCAPE:
						End();
						break;
					case SDLK_F7:
						showfps = !showfps;
						break;
					default:
						SCENE->KeyDown(e.key.keysym.sym, e.key.repeat);
					}
				}
				else if (e.type == SDL_KEYUP) {
					switch (e.key.keysym.sym) {
					default:
						SCENE->KeyUp(e.key.keysym.sym);
					}
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					// TODO
				}
				else if (e.type == SDL_MOUSEBUTTONUP) {
					// TODO
				}
				else if (e.type == SDL_MOUSEMOTION) {
					// TODO
				}
				else if (e.type == SDL_MOUSEWHEEL) {
					// TODO
				}
			}

			/*
			 * Graphic rendering
			 * - skin and movie(Image::Refresh) part are detached from main thread
			 *   so keypress will be out of lag
			 *   (TODO)
			 */
			SDL_RenderClear(RENDERER);
			SCENE->Update();
			SCENE->Render();
			if (showfps) Render_FPS();
			SDL_RenderPresent(RENDERER);
		}
	}

	void End() {
		/* simple :D */
		EndScene(SCENE);
		bRunning = false;
	}

	void Release() {
		// stop Bms loading && release Bms
		BmsHelper::ReleaseAll();

		// save game settings ...
		GameSettingHelper::SaveSetting(SETTING);

		// other scenes
		ReleaseScene(GamePlay::SCENE);

		// release basic instances
		delete LUA;

		// finally, game engine (audio/renderer) release
		Mix_CloseAudio();
		SDL_DestroyRenderer(RENDERER);
		SDL_DestroyWindow(WINDOW);
	}
}
