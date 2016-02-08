/*
 * GamePlay
 *
 * resources about playing game
 *
 * input:
 * (global; game setting)
 * (global; player setting)
 *
 * output:
 * (global; player setting)
 * (global; player score result)
 *
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

	/*
	 * @brief
	 * parameter for playing game
	 */
	struct PARAMETER {
		/* well.. bms related part will be changed into SongInfo struct, later. */
		RString bmspath[10];
		// requires in case of replay
		RString bmshash[10];
		// if course play, then play as much as that count.
		// if not, just count 1.
		int courseplay;
		int op1;	// 0x0000ABCD; RANDOM / SC / LEGACY(MORENOTE/ALL-LN) / JUDGE
		int op2;
		double rate;
		int rseed;

		// program option (not included in replay)
		int startmeasure;
		int endmeasure;
		int repeat;
		bool bga;
		bool replay;
		bool autoplay;
		double pacemaker;
	};
	extern PARAMETER	P;

	extern ScenePlay*	SCENE;
};