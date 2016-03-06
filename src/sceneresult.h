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

class SceneResult : public SceneBasic {
public:
	// set arguments
	int m_Roundcnt;
	int m_Round;

public:
	// basics
	virtual void Initialize();
	virtual void Start();
	virtual void Update();
	virtual void Render();
	virtual void End();
	virtual void Release();
};

extern SceneResult	*SCENERESULT;