#include "game.h"
#include "gameplay.h"
#include "gameresult.h"

#include "luamanager.h"
#include "util.h"
#include "Pool.h"
#include "font.h"
#include "file.h"
#include "version.h"
#include "tinyxml2.h"
#include "bmsresource.h"
#include "playerinfo.h"
#include "player.h"
#include "logger.h"

using namespace tinyxml2;

GameSetting		SETTING;
SceneBasic*		SCENE = NULL;
SDL_Window*		WINDOW = NULL;
IDisplay*		DISPLAY = NULL;

namespace Game {
	/*
	 * variables
	 */
	// game status
	bool			bRunning = false;	// is game running?
	std::mutex		RMUTEX;
	PARAMETER		P;

	// SDL
	SDL_Joystick*	JOYSTICK[10] = { 0, };
	int				nJoystickCnt = 0;

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
		 * COMMENT: after initization finished, reset scene timer.
		 */
		s->Start();
		oninputstart->Start();
		onscene->Start();
	}

	void ChangeScene(SceneBasic *s) {
		if (SCENE != s) {
			if (SCENE) EndScene(SCENE);
			StartScene(s);
		}
		SCENE = s;
	}

	/* 
	 * registering basic lua function start 
	 * TODO: generating rendering object (after we're done enough)
	 */
	namespace {
		int SetTimer(Lua* l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			SWITCH_OFF(key);
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
			Timer* t = SWITCH_GET(key);
			if (t && t->IsStarted())
				lua_pushboolean(l, 1);
			else
				lua_pushboolean(l, 0);
			return 1;
		}

		int GetTime(Lua *l) {
			RString key = (const char*)lua_tostring(l, -1);
			lua_pop(l, 1);
			Timer *t = SWITCH_GET(key);
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

	void LoadOption() {
		/*
		 * Load basic setting file ...
		 */
		if (!GameSettingHelper::LoadSetting(SETTING)) {
			LOG->Warn("Cannot load settings files... Use Default settings...");
			GameSettingHelper::DefaultSetting(SETTING);
		}
	}

	bool Initialize() {
		/*
		 * Basic instances initalization
		 */
		GameTimer::Tick();
		LUA = new LuaManager();
		RegisterBasicLuaFunction();

		/*
		 * Game engine initalize
		 */
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
			LOG->Critical("Failed to SDL_Init() ...");
			return -1;
		}
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {	// no lag, no sound latency
			LOG->Critical("Failed to Open Audio ...");
			return -1;
		}
		Mix_AllocateChannels(1296);
		int flag = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
		if (SETTING.fullscreen == 1)
			flag |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		else if (SETTING.fullscreen > 1)
			flag |= SDL_WINDOW_FULLSCREEN;
		if (SETTING.resizable)
			flag |= SDL_WINDOW_RESIZABLE;
		if (SETTING.vsync)
			flag |= SDL_RENDERER_PRESENTVSYNC;
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		WINDOW = SDL_CreateWindow(PROGRAMNAME " - " PROGRAMDATE "(" PROGRAMCOMMIT ")", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			SETTING.width, SETTING.height, flag);
		if (!WINDOW) {
			LOG->Critical("Failed to create window");
			return -1;
		}
		
		DISPLAY = new DisplaySDLGlew(WINDOW);
		if (!DISPLAY->Initialize(SETTING.width, SETTING.height)) {
			LOG->Critical("Failed to create Renderer");
			LOG->Critical(SDL_GetError());
			return -1;
		}
		printf("OpenGL Version %s\n", DISPLAY->GetInfo());

		nJoystickCnt = SDL_NumJoysticks();
		if (nJoystickCnt > 10) nJoystickCnt = 10;
		for (int i = 0; i < nJoystickCnt; i++) {
			JOYSTICK[i] = SDL_JoystickOpen(i);
		}

		/*
		 * prepare game basic resource
		 */
		TEXPOOL->Register("_black", SurfaceUtil::CreateColorTexture(0x000000FF));
		TEXPOOL->Register("_white", SurfaceUtil::CreateColorTexture(0xFFFFFFFF));
		RString path;
		path = "../system/resource/fastslow.png";
		FileHelper::ConvertPathToSystem(path);
		TEXPOOL->Register("_fastslow", SurfaceUtil::LoadTexture(path));
		path = "../system/resource/number_float.png";
		FileHelper::ConvertPathToSystem(path);
		TEXPOOL->Register("_number_float", SurfaceUtil::LoadTexture(path));
		oninputstart = SWITCH_GET("OnInputStart");
		onscene = SWITCH_GET("OnScene");
		font = FONTPOOL->LoadTTFFont("_system", 
			"../system/resource/NanumGothicExtraBold.ttf", 28, 0x909090FF, 0,
			0x000000FF, 1, 0,
			"../system/resource/fontbackground_small.png");

		/*
		* Scene instance initalization
		* (MUST after graphic initalization finished)
		*/
		InitalizeScene(&GamePlay::SCENE);
		InitalizeScene(&GameResult::SCENE);

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

	//
	// for checking joystick input
	//
	namespace {
		std::map<int, bool> pressing;
		void Press(int code) {
			pressing[code] = true;
		}
		bool IsPressing(int code) {
			return pressing[code];
		}
		void Up(int code) {
			pressing[code] = false;
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
					switch (e.key.keysym.scancode) {
					case SDL_SCANCODE_ESCAPE:
						End();
						break;
					case SDL_SCANCODE_F7:
						showfps = !showfps;
						break;
					default:
						SCENE->KeyDown(e.key.keysym.scancode, e.key.repeat);
					}
				}
				else if (e.type == SDL_KEYUP) {
					switch (e.key.keysym.scancode) {
					default:
						SCENE->KeyUp(e.key.keysym.scancode);
					}
				}
				else if (e.type == SDL_JOYBUTTONDOWN) {
					//e.jbutton.which
					int id = 1001 + e.jbutton.button;
					SCENE->KeyDown(id, IsPressing(id));
					Press(id);
				}
				else if (e.type == SDL_JOYBUTTONUP) {
					int id = 1001 + e.jbutton.button;
					SCENE->KeyUp(id);
					Up(id);
				}
				else if (e.type == SDL_JOYAXISMOTION) {
					// 0: left / right
					// - 1020: left
					// - 1021: right
					// 1/2: up / down
					// - 1022: up
					// - 1023: down
#define JOYSTICKPRESS(id)\
	SCENE->KeyDown(id, IsPressing(id)); Press(id);
#define JOYSTICKUP(id)\
	if (IsPressing(id)) { SCENE->KeyUp(id); Up(id); }
					if (e.jaxis.axis == 0) {
						if (e.jaxis.value >= 8000) {
							JOYSTICKPRESS(1020);
							JOYSTICKUP(1021);
						}
						else if (e.jaxis.value <= -8000) {
							JOYSTICKUP(1020);
							JOYSTICKPRESS(1021);
						}
						else {
							JOYSTICKUP(1020);
							JOYSTICKUP(1021);
						}
					}
					else {
						if (e.jaxis.value >= 8000) {
							JOYSTICKPRESS(1022);
							JOYSTICKUP(1023);
						}
						else if (e.jaxis.value <= -8000) {
							JOYSTICKUP(1022);
							JOYSTICKPRESS(1023);
						}
						else {
							JOYSTICKUP(1022);
							JOYSTICKUP(1023);
						}
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
			RMUTEX.lock();
			
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();

			glBegin(GL_QUADS);
			glColor3f(1, 0, 0); glVertex3f(0, 0, 0);
			glColor3f(1, 1, 0); glVertex3f(100, 0, 0);
			glColor3f(1, 0, 1); glVertex3f(100, 100, 0);
			glColor3f(1, 1, 1); glVertex3f(0, 100, 0);
			glEnd();

			SCENE->Update();
			SCENE->Render();
			if (showfps) Render_FPS();

			//SDL_RenderPresent(RENDERER);
			//glFinish();
			SDL_GL_SwapWindow(WINDOW);
			RMUTEX.unlock();
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
		ReleaseScene(&GamePlay::SCENE);
		ReleaseScene(&GameResult::SCENE);

		// release basic instances
		delete LUA;

		// finally, game engine (audio/renderer/joystick/etc...) release
		for (int i = 0; i < nJoystickCnt; i++)
			SDL_JoystickClose(JOYSTICK[i]);
		Mix_CloseAudio();
		delete DISPLAY;
		SDL_DestroyWindow(WINDOW);
	}
}
