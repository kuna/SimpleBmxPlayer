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
#include "Timer.h"


class ScenePlay : public SceneBasic {
public:
	// play related
	int					m_MinLoadingTime = 3000;
	int					m_ReadyTime = 1000;
	int					playmode;			// PLAYTYPE..?
	double				m_Playerhealth[3];	// Stored gauge

	// 1P / 2P ...??
	RString				m_Songpath;
	RString				m_Songhash;

	// game skin
	Theme				theme;

	// theme metrics (internally used)
	Value<int>			vRivalDiff;
	SwitchValue			OnReady;
	SwitchValue			OnClose;			// when 1p & 2p dead

	SwitchValue			OnCourseRound[10];
	SwitchValue			OnCourse, OnExpert, OnDemo, OnGrade;
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