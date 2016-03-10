#include "game.h"
#include "version.h"
#include "Setting.h"

#include "Luamanager.h"
#include "Input.h"
#include "util.h"
#include "Theme.h"
#include "font.h"
#include "file.h"
#include "Song.h"
#include "SongPlayer.h"
#include "tinyxml2.h"
#include "Profile.h"
#include "player.h"
#include "logger.h"

using namespace tinyxml2;

SceneManager*	SCENE = NULL;
SDL_Window*		WINDOW = NULL;
GameState		GAMESTATE;
Parameter		PARAMETER;

namespace Game {
	bool			bRunning = false;	// is game running?

	// for sys input receiver
	class SysInputReceiver : public InputReceiver {
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
				SCENE->ShowFPSToggle();
				break;
			default:
			}
		}
	};
	SysInputReceiver	m_BasicInput;



	bool Initialize() {
		/*
		* Initalize at very first
		* (Pool/FileManager is automatically initalized, so we don't need to take care of it)
		*/
		LUA = new LuaManager();

		/*
		* Pool need to be initialized before resource(display) registration
		*/
		PoolHelper::InitializeAll();

		/*
		 * Create window
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
		
		/*
		 * Create Display
		 */
		DISPLAY = new DisplaySDLGlew(WINDOW);
		if (!DISPLAY->Initialize(SETTING.width, SETTING.height)) {
			LOG->Critical("Failed to create Renderer");
			LOG->Critical(SDL_GetError());
			return -1;
		}
		LOG->Info("OpenGL Version %s\n", DISPLAY->GetInfo());

		/*
		 * Scene must be initalized after all elements are initialized
		 * (uses pool, lua, display[font])
		 */
		SCENE = new SceneManager();
		INPUT = new InputManager();
		SONGPLAYER = new SongPlayer();
		SONGMANAGER = new SongManager();
		PROFILE[0] = new Profile(0);
		PROFILE[1] = new Profile(1);

		/*
		 * some etc initialization
		 */
		INPUT->Register(&m_BasicInput);

		/*
		 * Process parameter before scene start
		 */
		for (auto it = PARAMETER.option_cmds.begin(); it != PARAMETER.option_cmds.end(); ++it) {
			PROFILE[0]->GetPlayOption()->ParseOptionString(*it);
		}

		return true;
	}

	void Release() {
		// stop Bms loading && release Bms
		BmsHelper::ReleaseAll();

		// save game settings ...
		GameSettingHelper::SaveSetting(SETTING);

		// release basic instances
		// COMMENT: Font/Surface pool should be destroyed first before DISPLAY destroyed
		// So order is important.
		delete PROFILE[0];
		delete PROFILE[1];
		delete SONGMANAGER;
		delete SONGPLAYER;
		delete SCENE;
		delete INPUT;
		PoolHelper::ReleaseAll();
		delete DISPLAY;
		delete LUA;
		Mix_CloseAudio();
		SDL_DestroyWindow(WINDOW);
	}

	void MainLoop() {
		while (bRunning) {
			/*
			 * Sound is processed on other thread (polling system),
			 * So we don't take care of it now.
			 */

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

			DISPLAY->EndRender();
		}
	}

	void End() {
		bRunning = false;
	}
}






#include "sceneplay.h"
#include "sceneresult.h"

void SceneManager::Initalize() {
	m_bShowFPS = false;

	// theme metrics
	m_Uptime.SetFromPool("Game");
	m_Rendertime.SetFromPool("Render");
	m_Scenetime.SetFromPool("Scene");
	m_Fadeout.SetFromPool("FadeOut");
	m_Fadein.SetFromPool("FadeIn");
	m_Uptime.Start();

	// basic resource setting
	Reload();

	// register scene
	RegisterScene("Play", new ScenePlay());
	RegisterScene("Result", new SceneResult());

	// initalize input
	INPUT->Register(&m_Input);

	// getcha font
	m_Basefnt = FONTPOOL->GetByID("_system");
}

void SceneManager::Release() {
	// unregister current scene
	if (m_FocusedScene)
		INPUT->UnRegister(m_FocusedScene);

	// delete all scenes
	for (auto it = m_Scenes.begin(); it != m_Scenes.end(); ++it)
		delete it->second;
	m_Scenes.clear();

	// unregister input
	INPUT->UnRegister(&m_Input);
}

void SceneManager::Update() {
	// if next scene waiting?
	// if then, change scene now.
	if (m_NextScene && m_Fadeout.GetTick() >= m_NextSceneTime) {
		m_Fadeout.Stop();
		if (m_FocusedScene) INPUT->UnRegister(m_FocusedScene);
		if (m_NextScene) {
			m_FocusedScene->End();
			INPUT->Register(m_NextScene);
			m_NextScene->Start();
			m_Scenetime.Start();
		}
		m_FocusedScene = m_NextScene;
		m_NextScene = 0;
	}

	// update current/fg/bg scene
	if (m_FocusedScene) {
		m_FocusedScene->Update();
	}
	for (auto it = m_SceneBackground.begin(); it != m_SceneBackground.end(); ++it)
		(*it)->Update();
	for (auto it = m_SceneForeground.begin(); it != m_SceneForeground.end(); ++it)
		(*it)->Update();

	// just call trigger that we're updating scene
	m_Rendertime.Start();
}

void SceneManager::Render() {
	// Render background scene (TODO)
	DISPLAY->PushState();
	DISPLAY->SetOffset(0, 0);
	DISPLAY->SetZoom(0.5, 0.5);
	for (auto it = m_SceneBackground.begin(); it != m_SceneBackground.end(); ++it)
		(*it)->Render();
	DISPLAY->PopState();

	// Render current scene
	if (m_FocusedScene) {
		m_FocusedScene->Render();
	}

	// Render foreground scene (TODO)
	DISPLAY->PushState();
	DISPLAY->SetOffset(0, 0);
	DISPLAY->SetZoom(0.5, 0.5);
	for (auto it = m_SceneForeground.begin(); it != m_SceneForeground.end(); ++it)
		(*it)->Render();
	DISPLAY->PopState();

	// Render FPS
	int curtime = m_Uptime.GetTick();
	int deltatime = curtime - m_prevtime;
	if (deltatime > 0) {
		m_Basefnt->Render(ssprintf("%.0f Frame", 1000.0f / deltatime), 10, 20);
	}
	m_prevtime = curtime;

	// Render System message
	if (m_MessageTimer.GetTick() > 5000) {
		m_Basefnt->Render(m_Message, 10, 50);
	}
}

/* static */
SceneBasic* SceneManager::GetScene(const RString& name) {
	auto it = m_Scenes.find(name);
	if (it == m_Scenes.end())
		return 0;
	else
		return it->second;
}

/* static */
void SceneManager::RegisterScene(const RString& name, SceneBasic* scene) {
	if (GetScene(name)) {
		LOG->Warn("Scene `%s` Already exists, Cannot add.", name.c_str());
		return;
	}
	m_Scenes[name] = scene;
}

void SceneManager::ChangeScene(const RString& name) {
	// an shortcut
	ChangeSceneAfterTime(name, 0);
}

void SceneManager::ChangeSceneAfterTime(const RString& name, int timeout) {
	SceneBasic* scene = GetScene(name);
	if (!scene) {
		LOG->Warn("Cannot find scene(%s). Nothing will be drawn!", name);
	}

	/*
	* COMMENT: scene translating might be called during rendering or updating or somewhat ...
	* but scene MUST not be changed during Updating, because that scene should be rendered, not new scene.
	* So, store it m_NextScene (not m_FocusedScene) and render it next time.
	*/
	m_NextScene = scene;

	/*
	* bg / fg are automatically cleared
	*/
	m_SceneBackground.clear();
	m_SceneForeground.clear();

	/* Activate fadeout timer */
	m_NextSceneTime = timeout;
	m_Fadeout.Start();
}

bool SceneManager::IsChangingScene() {
	return m_Fadeout.IsStarted();
}

void SceneManager::AddBackgroundScene(const RString& name) {
	SceneBasic* scene = GetScene(name);
	if (!scene) {
		LOG->Warn("Attempt to push empty scene as background - why?");
		return;
	}
	m_SceneBackground.push_back(scene);
}

void SceneManager::AddForegroundScene(const RString& name) {
	SceneBasic* scene = GetScene(name);
	if (!scene) {
		LOG->Warn("Attempt to push empty scene as foreground - why?");
		return;
	}
	m_SceneForeground.push_back(scene);
}

void SceneManager::SetBackgroundSceneDST(const Display::Rect& r) {
	m_SceneBgDST = r;
}

void SceneManager::SetForegroundSceneDST(const Display::Rect& r) {
	m_SceneFgDST = r;
}

void SceneManager::ShowFPSToggle() { m_bShowFPS = !m_bShowFPS; }

void SceneManager::SetScreenMessage(const RString& msg) {
	m_Message = msg;
	m_MessageTimer.Start();
}

void SceneManager::Reload() {
	// Clear all resources
	Theme::ClearAllResource();
	TEXPOOL->ReleaseAll();
	FONTPOOL->ReleaseAll();

	// Register basic resources
	TEXPOOL->Register("_black", SurfaceUtil::CreateColorTexture(0x000000FF));
	TEXPOOL->Register("_white", SurfaceUtil::CreateColorTexture(0xFFFFFFFF));
	//TEXPOOL->Register("_fastslow", SurfaceUtil::LoadTexture("../system/resource/fastslow.png"));
	//TEXPOOL->Register("_number_float", SurfaceUtil::LoadTexture("../system/resource/number_float.png"));
	FONTPOOL->LoadTTFFont("_system",
		"../system/resource/NanumGothicExtraBold.ttf", 28, 0x909090FF, 0,
		0x000000FF, 1, 0,
		"../system/resource/fontbackground_small.png");

	// Reload currently activated scene's resource
	if (m_FocusedScene) m_FocusedScene->Reload();
	for (auto it = m_SceneBackground.begin(); it != m_SceneBackground.end(); ++it)
		(*it)->Reload();
	for (auto it = m_SceneForeground.begin(); it != m_SceneForeground.end(); ++it)
		(*it)->Reload();
}