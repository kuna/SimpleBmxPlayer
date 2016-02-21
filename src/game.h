#pragma once

#include "Display.h"
#include "gamesetting.h"
#include "playerinfo.h"
#include <tchar.h>
#include <mutex>

class SceneBasic {
public:
	// basics
	virtual void Initialize() {};
	virtual void Start() {};
	virtual void Update() {};
	virtual void Render() {};
	virtual void End() {};
	virtual void Release() {};

	// event handler
	virtual void KeyUp(int code) {};
	virtual void KeyDown(int code, bool repeating) {};
	virtual void MouseUp(int x, int y) {};
	virtual void MouseDown(int x, int y) {};
	virtual void MouseMove(int x, int y) {};
	virtual void MouseStartDrag(int x, int y) {};
	virtual void MouseDrag(int x, int y) {};
	virtual void MouseEndDrag(int x, int y) {};
};

namespace Game {
	// game's main 3 loop: init, mainloop, release
	/*
	 * @brief load game option
	 * (must called before initalization & after argument parsing)
	 */
	void LoadOption();
	/** @brief Initalize routine for game. */
	bool Initialize();
	/** @brief mainly, fetchs event / render */
	void MainLoop();
	/** @brief It isn't necessary to be called, but must called from somewhere else to exit MainLoop() */
	void End();
	/** @brief release all resources before program terminates. */
	void Release();

	// game scene transition
	/** @brief call OnScene/OnInputStart */
	void StartScene();
	/** @brief call StartScene() if scene is different */
	void ChangeScene(SceneBasic *s);

	// global veriables about game rendering.
	extern std::mutex		RMUTEX;

	struct PARAMETER {
		// well, main parameter don't has much things to do
		std::string username;
		// SC, JUDGE, LEGACY LN,
		// - let's set them later. there're much more significant things.
	};
	extern PARAMETER		P;
}

extern SceneBasic*		SCENE;
extern IDisplay*		DISPLAY;
extern SDL_Window*		WINDOW;
extern GameSetting		SETTING;