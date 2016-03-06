#pragma once

#include "Input.h"
#include "Display.h"
#include "playerinfo.h"

namespace Game {
	/** @brief Initalize routine for game. */
	bool Initialize();
	/** @brief mainly, fetchs event / render */
	void MainLoop();
	/** @brief release all resources before program terminates. */
	void Release();
	/** @brief It isn't necessary to be called, but must called from somewhere else to exit MainLoop() */
	void End();
}

extern IDisplay*		DISPLAY;
extern SDL_Window*		WINDOW;




/* Scene part */

class SceneBasic : public InputReceiver {
public:
	// basics
	virtual void Initialize() {};
	virtual void Start() {};
	virtual void Update() {};
	virtual void Render() {};
	virtual void End() {};
	virtual void Release() {};
};

class SceneManager {
	// all cached scenes
	static std::map<RString, SceneBasic*> m_Scenes;
	// scene backgrounds / foregrounds (mostly not used)
	std::vector<SceneBasic*> m_SceneBackground;		// e.g. - Theme change preview
	std::vector<SceneBasic*> m_SceneForeground;		// e.g. - like stepmania #FGCHANGE
	Display::Rect m_SceneBgDST;
	Display::Rect m_SceneFgDST;
	SceneBasic* m_FocusedScene = 0;					// Currently running scene (input event only triggers here)
	SceneBasic* m_NextScene = 0;

	// wanna some additional information?
	bool m_bShowFPS;
	int m_prevtime;
	RString m_Message;
	Timer m_MessageTimer;
	Font *m_Basefnt;		// base font for rendering text

	InputReceiver m_Input;

	void Initalize();
	void Release();
public:
	SceneManager() { Initalize(); };
	~SceneManager() { Release(); };

	void Update();
	void Render();

	static void RegisterScene(const RString& name, SceneBasic* scene);
	static SceneBasic* GetScene(const RString& name);
	void ChangeScene(const RString& name);
	void AddBackgroundScene(const RString& name);
	void AddForegroundScene(const RString& name);
	void SetBackgroundSceneDST(const Display::Rect& r);
	void SetForegroundSceneDST(const Display::Rect& r);

	void ShowFPSToggle();
	void SetScreenMessage(const RString& msg);
	void Reload();								// Reload all scenes (only in background / overlay / focused) (COSTS A LOT! be warned.)
};

extern SceneManager* SCENE;