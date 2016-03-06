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
	// currently playing songpath / hash.
	// (1P / 2P)?
	RString m_SongPath;
	RString m_SongHash;

	// record disabled
	// in case of training mode / replay
	bool m_IsRecordable;

	int op1;			// 0x0000ABCD; RANDOM / SC / LEGACY(MORENOTE/ALL-LN) / JUDGE
	int op2;
	double rate;
	int rseed;

	// play related
	int					playmode;			// PLAYTYPE..?
	PlayerSongRecord	record;


	// game skin
	Theme				theme;

	//
	// theme metrics (internally used)
	//
	Timer*			OnSongStart;
	Timer*			OnSongLoading;
	Timer*			OnSongLoadingEnd;
	Timer*			OnReady;
	Timer*			OnClose;			// when 1p & 2p dead
	Timer*			OnFadeIn;			// when game start
	Timer*			OnFadeOut;			// when game end
	Timer*			On1PMiss;			// just for missing image
	Timer*			On2PMiss;			// just for missing image

	int*			P1RivalDiff;
	double*			P2ExScore;
	double*			P2ExScoreEsti;
public:
	// basics
	virtual void Initialize();
	virtual void Start();
	virtual void Update();
	virtual void Render();
	virtual void End();
	virtual void Release();

	// event handler
	virtual void OnUp(int code);
	virtual void OnDown(int code);
};