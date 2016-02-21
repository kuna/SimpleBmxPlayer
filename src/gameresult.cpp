#include "gameresult.h"
#include "gameplay.h"

namespace GameResult {
	SceneResult		SCENE;
	PARAMETER		P;

	void SceneResult::Initialize() {

	}

	void SceneResult::Start() {
		// decide to finish or go to game scene
		if (P.round < P.roundcnt) {
			GamePlay::P.round = P.round + 1;
			Game::ChangeScene(&GamePlay::SCENE);
		}
		else {
			Game::End();
		}
	}

	void SceneResult::Update() {
		//
	}

	void SceneResult::Render() {
		// don't do anything
	}

	void SceneResult::End() {
		// 
	}

	void SceneResult::Release() {
		//
	}
}