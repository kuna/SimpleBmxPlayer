#pragma once

#include "Display.h"
#include "playerinfo.h"
#include <tchar.h>

class SceneBasic: public InputReceiver {
public:
	// basics
	virtual void Initialize() {};
	virtual void Start() {};
	virtual void Update() {};
	virtual void Render() {};
	virtual void End() {};
	virtual void Release() {};
};

namespace Game {
	/** @brief Initalize routine for game. */
	bool Initialize();
	/** @brief mainly, fetchs event / render */
	void MainLoop();
	/** @brief release all resources before program terminates. */
	void Release();
	/** @brief It isn't necessary to be called, but must called from somewhere else to exit MainLoop() */
	void End();

	// game scene transition
	/** @brief call OnScene/OnInputStart */
	void StartScene();
	/** @brief call StartScene() if scene is different */
	void ChangeScene(SceneBasic *s);
}

extern SceneBasic*		SCENE;
extern IDisplay*		DISPLAY;
extern SDL_Window*		WINDOW;