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

#include "game.h"
#include "Theme.h"
#include "timer.h"


class ScenePlay : public SceneBasic {
public:
	/* well.. bms related part will be changed into SongInfo struct, later. */
	RString bmspath[10];
	// requires in case of replay
	RString bmshash[10];
	// if course play, then play as much as that count.
	// if not, just count 1.
	int courseplay;
	// round of courseplay (start from 1)
	int round;
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
	bool m_Autoplay;
	double pacemaker;

	// record disabled
	// in case of training mode / replay
	bool isrecordable;

	// game skin
	Theme				theme;

	// play related
	int					playmode;			// PLAYTYPE..?
	PlayerSongRecord	record;

public:
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

extern ScenePlay*	SCENEPLAY;