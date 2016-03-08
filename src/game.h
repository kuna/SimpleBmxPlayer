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

// globally used variables related to game
// (Used as Scene's Initalization Parameter)
class GameState {
public:
	GameState();

	// basic related
	RString m_username;

	// select related

	// playing related
	RString m_CoursePath[10];
	RString m_CourseHash[10];
	int m_CourseRound;
	int m_CourseCount;
	RString m_2PSongPath[10];	// not generally used
	RString m_2PSongHash[10];	// not generally used

	int m_Startmeasure;
	int m_Endmeasure;
	int m_SongRepeatCount;
	bool m_ShowBga;
	int m_rseed;
	int m_Battle;			// set battle mode (OFF, BATTLE, G-BATTLE, ...)
	int m_Keymode;
	double m_PlayRate;
	struct PlayerState {
		int playertype;
		int op;				// 0x0000ABCD; RANDOM / SC / LEGACY(MORENOTE/ALL-LN) / JUDGE
		double gauge;		// Store gauge state in here (Use for taken of that ...?)
	} m_Player[2];
	double m_PacemakerGoal;
};

extern SDL_Window*		WINDOW;		// main window
extern GameState		GAMESTATE;	// game's main state




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
	virtual void Reload() {};
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
	int m_NextSceneTime = 0;

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
	// theme metrics
	SwitchValue m_Uptime;
	SwitchValue m_Scenetime;
	SwitchValue m_Rendertime;
	SwitchValue m_Fadeout;
	SwitchValue m_Fadein;

public:
	SceneManager() { Initalize(); };
	~SceneManager() { Release(); };

	void Update();
	void Render();

	static void RegisterScene(const RString& name, SceneBasic* scene);
	static SceneBasic* GetScene(const RString& name);
	void ChangeScene(const RString& name);
	void ChangeSceneAfterTime(const RString& name, int timeout);
	bool IsChangingScene();
	void AddBackgroundScene(const RString& name);
	void AddForegroundScene(const RString& name);
	void SetBackgroundSceneDST(const Display::Rect& r);
	void SetForegroundSceneDST(const Display::Rect& r);

	void ShowFPSToggle();
	void SetScreenMessage(const RString& msg);
	void Reload();								// Reload all scenes (only in background / overlay / focused) (COSTS A LOT! be warned.)
};

extern SceneManager* SCENE;