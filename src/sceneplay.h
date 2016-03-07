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

	// play related
	int					m_MinLoadingTime = 3000;
	int					m_ReadyTime = 1000;
	int					playmode;			// PLAYTYPE..?
	PlayerSongRecord	record;


	// game skin
	Theme				theme;

	//
	// theme metrics (internally used)
	//
	SwitchValue			m_DiffSwitch[6];
	Value<int>			m_PlayLevel;

	SwitchValue			OnReady;
	SwitchValue			OnClose;			// when 1p & 2p dead
	SwitchValue			OnFadeIn;			// when game start
	SwitchValue			OnFadeOut;			// when game end

	Value<int>			P1RivalDiff;
	Value<double>		P2ExScore;
	Value<double>		P2ExScoreEsti;
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