/*
 * GamePlay
 *
 * resources about playing game
 */

#pragma once
#include "SDL/SDL.h"
#include "game.h"
#include "timer.h"

/*
* @describes
* stores all variables about player skin rendering value
* initalized at ScenePlay::Initialize()
*/
typedef struct {
	Timer*				pOnMiss;			// timer used when miss occured (DP)
	Timer*				pOnCombo;
	Timer*				pOnJudge[6];		// pf/gr/gd/bd/pr
	Timer*				pOnfullcombo;		// needless to say?
	Timer*				pOnlastnote;		// when last note ends
	Timer*				pOnGameover;		// game is over! (different from OnClose)
	Timer*				pOnGaugeMax;		// guage max?
	Timer*				pLanepress[10];
	Timer*				pLanehold[10];
	Timer*				pLaneup[10];
	Timer*				pLanejudgeokay[10];
	double*				pExscore_d;
	double*				pHighscore_d;
	int*				pScore;
	int*				pExscore;
	int*				pCombo;
	int*				pMaxCombo;
	int*				pTotalnotes;
	int*				pRivaldiff;		// TODO where to process it?
	double*				pGauge_d;
	int*				pGaugeType;
	int*				pGauge;
	double*				pRate_d;
	int*				pRate;
	double*				pTotalRate_d;
	int*				pTotalRate;
	int*				pNoteSpeed;
	int*				pFloatSpeed;
	int*				pSuddenHeight;
	int*				pLiftHeight;
} PlayerRenderValue;

/** @brief stores rendering variables related to Players */
extern PlayerRenderValue	 PLAYERVALUE[4];

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