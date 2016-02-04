/*
 * GamePlay
 *
 * resources about playing game
 */

#pragma once
#include "SDL/SDL.h"
#include "game.h"
#include "timer.h"

namespace GamePlay {
	class ScenePlay : public SceneBasic {
		// basics
		virtual void Initialize();
		virtual void Start();
		virtual void Update();
		virtual void Render();
		virtual void End();
		virtual void Release();

		// event handler
		virtual void KeyUp(int code);
		virtual void KeyDown(int code, bool repeating);
		//virtual void MouseUp(int x, int y);
		//virtual void MouseDown(int x, int y);
		//virtual void MouseMove(int x, int y);
		//virtual void MouseStartDrag(int x, int y);
		//virtual void MouseDrag(int x, int y);
		//virtual void MouseEndDrag(int x, int y);
	};

	extern ScenePlay*	SCENE;
};