/*
 * GamePlay
 *
 * resources about playing game
 */

#pragma once
#include "player.h"
#include "SDL/SDL.h"

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
	void Init();
	void Start();
	void Render();
	void Release();
};