#include "game.h"

/*
 * Scene Result
 *
 * currently this scene doesn't do anything
 * only exists for courseplay
 * (decide to end game or go for next round ...)
 *
 * input
 * (current round / total round)
 * (global; player score)
 *
 * output
 * (none)
 *
 */

namespace GameResult {
	class SceneResult : public SceneBasic {
		// basics
		virtual void Initialize();
		virtual void Start();
		virtual void Update();
		virtual void Render();
		virtual void End();
		virtual void Release();
	};

	struct PARAMETER {
		int roundcnt;
		int round;
	};

	extern PARAMETER	P;
	extern SceneResult	SCENE;
}