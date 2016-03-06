#include "Sceneresult.h"

void SceneResult::Initialize() {

}

void SceneResult::Start() {
	// decide to finish or go to game scene
	if (m_Round < m_Roundcnt) {
		SCENE->ChangeScene("Play");
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