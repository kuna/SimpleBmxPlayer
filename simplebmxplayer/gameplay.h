/*
 * GamePlay
 *
 * resources about playing game
 */

#pragma once
#include "player.h"
#include "SDL/SDL.h"
#include "game.h"

namespace {
	enum PLAYTYPE {
		PLAY_5KEY = 5,
		PLAY_7KEY = 7,
		PLAY_9KEY = 9,
		PLAY_10KEY = 10,
		PLAY_14KEY = 14,
		PLAY_18KEY = 18,
	};
}

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
		virtual void MouseUp(int x, int y);
		virtual void MouseDown(int x, int y);
		virtual void MouseMove(int x, int y);
		virtual void MouseStartDrag(int x, int y);
		virtual void MouseDrag(int x, int y);
		virtual void MouseEndDrag(int x, int y);
	};

	extern ScenePlay*	SCENE;
};