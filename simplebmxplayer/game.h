#include "SDL/SDL.h"
#include "gamesetting.h"
#include "playerinfo.h"
#include <tchar.h>

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
	extern SceneBasic*		SCENE;
	extern SDL_Renderer*	RENDERER;
	extern SDL_Window*		WINDOW;
	extern GameSetting		SETTING;
}