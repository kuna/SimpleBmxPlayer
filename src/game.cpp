#include "game.h"
#include "gameplay.h"
#include "gameresult.h"
#include "Setting.h"

#include "Luamanager.h"
#include "Input.h"
#include "util.h"
#include "Pool.h"
#include "font.h"
#include "file.h"
#include "version.h"
#include "SongPlayer.h"
#include "tinyxml2.h"
#include "playerinfo.h"
#include "player.h"
#include "logger.h"

using namespace tinyxml2;

SceneBasic*		SCENE = NULL;
SDL_Window*		WINDOW = NULL;
IDisplay*		DISPLAY = NULL;

namespace Game {
	/*
	 * variables
	 */
	// game status
	bool			bRunning = false;	// is game running?

	// FPS
	Timer			fpstimer;
	int				fps = 0;
	Font			*font;				// system font
	bool			showfps = true;

	// basic
	Timer			*oninputstart;
	Timer			*onscene;


	bool Initialize() {
		GameTimer::Tick();
		/*
		 * Lua basic instances initalization
		 */
		LUA = new LuaManager();
		Lua *L = LUA->Get();
		LuaBinding<StringPool>::Register(L, 0, 0);
		LuaBinding<IntPool>::Register(L, 0, 0);
		LuaBinding<DoublePool>::Register(L, 0, 0);
		LuaBinding<HandlerPool>::Register(L, 0, 0);
		LUA->Release(L);

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
		LOG->Info("OpenGL Version %s\n", DISPLAY->GetInfo());


		/*
		 * prepare game basic resource
		 */
		TEXPOOL->Register("_black", SurfaceUtil::CreateColorTexture(0x000000FF));
		TEXPOOL->Register("_white", SurfaceUtil::CreateColorTexture(0xFFFFFFFF));
		TEXPOOL->Register("_fastslow", SurfaceUtil::LoadTexture("../system/resource/fastslow.png"));
		TEXPOOL->Register("_number_float", SurfaceUtil::LoadTexture("../system/resource/number_float.png"));
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
		delete INPUT;
		delete DISPLAY;
		Mix_CloseAudio();
		SDL_DestroyWindow(WINDOW);
	}

	// ---------------------------------------

	/*
	 * basic input handler
	 */
	class SceneManagerInput : public InputReceiver {
	public:
		virtual void OnSys(int code) {
			if (code = INPUTSYS::EXIT)
				End();
		}

		virtual void OnPress(int code) {
			switch (code) {
			case SDL_SCANCODE_ESCAPE:
				End();
				break;
			case SDL_SCANCODE_F7:
				showfps = !showfps;
				break;
			default:
			}
		}
	};
	SceneManagerInput GAMEINPUT;

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
			* process input
			*/
			INPUT->Update();


			/*
			* Graphic rendering
			* - skin and movie(Image::Refresh) part are detached from main thread
			*   so keypress will be out of lag
			*   (TODO)
			*/
			DISPLAY->BeginRender();

			glBegin(GL_QUADS);
			glColor3f(1, 0, 0); glVertex3f(0, 0, 0);
			glColor3f(1, 1, 0); glVertex3f(100, 0, 0);
			glColor3f(1, 0, 1); glVertex3f(100, 100, 0);
			glColor3f(1, 1, 1); glVertex3f(0, 100, 0);
			glEnd();

			SCENE->Update();
			SCENE->Render();
			if (showfps) Render_FPS();

			DISPLAY->EndRender();
		}
	}

	void End() {
		/* simple :D */
		EndScene(SCENE);
		bRunning = false;
	}
}
