/*
 * @description processor for each player.
 * player type: Normal(Basic) / CPU(Auto) / Replay
 * player contains: BGA status / Combo / Score Rate / Health / Note / Lain Lift / Speed
 */

#pragma once

#include "bmsbel/bms_bms.h"
#include "bmsinfo.h"
#include "playrecord.h"
#include "timer.h"
#include "image.h"
#include "global.h"
#include "playerinfo.h"

class SkinPlayObject;

/*
 * @description
 * Very basic form of Player
 * only stores data for real game play
 * (don't store data for each player; refer PlayerInfo/PlayerSongRecord)
 */
class Player {
protected:
	// stored/storing settings
	// COMMENT: we don't need laneheight.
	// we just need to get relative position of lane(0 ~ 1). 
	// skinelement will process it by itself.
	PlayerInfo			playerinfo;
	int					playside;
	int					playertype;
	double				speed;				// for calculating speed_mul
	double				speed_mul;			// absbeat * speed_mul = (real y pos)
	int					speed_type;			// TODO

	// some prefetch timers/values
	Timer*				bmstimer;			// elapsed time
	Timer*				on1pmiss;			// timer used when miss occured
	Timer*				on2pmiss;			// timer used when miss occured
	Timer*				lanepress[20];
	Timer*				lanehold[20];
	Timer*				laneup[20];
	Timer*				lanejudgeokay[20];
	Timer*				onfullcombo;
	Timer*				onlastnote;			// when last note ends
	Image*				currentmissbga;		// when miss occurs, get current Miss BGA
	double*				exscore_graph;
	double*				highscore_graph;
	int*				playscore;
	int*				playexscore;
	int*				playcombo;
	int*				playmaxcombo;
	int*				playtotalnotes;
	int*				playgrooveguage;
	int*				playrivaldiff;
	Timer*				on1pjudge;
	Timer*				on2pjudge;
	double*				playerguage;
	int*				playerguagetype;

	int					judgenotecnt;		// judged note count; used for OnLastNote

	// DON'T CHEAT! check for value malpulation.
	// (TODO) processed by CryptManager
#ifdef _CRYPTMANAGER
#endif

	// grade information
	PlayerGrade				grade;

	// current judge/bar/channel related information
	int					noteindex[20];					// currently processing note index(bar index)
	int					longnotestartpos[20];			// longnote start pos(bar index)
	std::vector<BmsWord> keysound[20];					// sounding key value (per bar index)
	double				health;							// player's health

	// note/time information
	BmsNoteContainer	bmsnote;
	int					keys;
	Uint32				currenttime;
	Uint32				currentbar;

	bool				IsLongNote(int notechannel);	// are you pressing longnote currently?

	// judge
	int CheckJudgeByTiming(double delta);
	// active note searcher
	int GetNextAvailableNoteIndex(int notechannel);
	BmsNote* GetCurrentNote(int notechannel);
	/** @brief make judgement. silent = true will not set JUDGE timer. */
	void MakeJudge(int judgetype, int channel, bool silent = false);
public:
	Player(int type = PLAYERTYPE::NORMAL);

	/*
	 * @brief 
	 * Generate Note object / 
	 * Must call before game play starts, or there'll be nothing(no note/play) during gameplay.
	 */
	void Prepare(int playside);
	/*
	 * @brief 
	 * Update late note/note position
	 * Uses Global Game(BmsResource) Timer
	 */
	virtual void Update();
	/*
	 * @brief
	 * Hard-Reset player's note position
	 * Not generally used, only used for starting from specific position.
	 */
	virtual void Reset();

	/** @brief General key input for judgement */
	virtual void UpKey(int keychannel);
	/** @brief General key input for judgement */
	virtual void PressKey(int keychannel);

	/** @brief How much elapsed from last MISS? effects to MISS BGA. */
	double GetLastMissTime();

	/** @brief renders note */
	void RenderNote(SkinPlayObject *);

	/** @Get/Set */
	double GetSpeed();
	void SetSpeed(double speed);
	PlayerGrade GetGrade();
	int GetCurrentBar();
	int GetCurrentNoteBar(int channel);
	bool IsNoteAvailable(int notechannel);
	int GetAvailableNoteIndex(int notechannel, int start = 0);
	bool IsDead();
	bool IsFinished();	// TODO
	void SetPlayerConfig(const PlayerPlayConfig& config);
};

/*
 * @description
 * Played by CPU
 * If this player activated, 'Autoplay' mark will show up.
 */
class PlayerAuto : public Player {
	double targetrate;

public:
	PlayerAuto();

	virtual void Update();
	virtual void PressKey(int channel);
	virtual void UpKey(int channel);

	void SetGoal(double rate);
};

/*
 * @description
 * Played from Replay data.
 * If this player activated, `Replay` mark will show up.
 */
class PlayerGhost : public Player {
public:
	PlayerGhost();
};

namespace PlayerHelper {
	/** @brief get key function from config. returns -1 if no key function attributes to that player. */
	int GetKeyFunction(const PlayerKeyConfig& keyconfig, int keycode);
	/** @brief load player info from player's name */
}