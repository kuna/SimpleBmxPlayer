/*
 * @description processor for each player.
 * player type: Normal(Basic) / CPU(Auto) / Replay
 * player contains: BGA status / Combo / Score Rate / Health / Note / Lain Lift / Speed
 */

#pragma once

#include "bmsbel/bms_bms.h"
#include "playrecord.h"
#include "timer.h"
#include "image.h"
#include "global.h"
#include "playerinfo.h"
#include "gameplay.h"

class SkinPlayObject;

/*
 * @description
 * Very basic form of Player
 * only stores data for real game play
 * (don't store data for each player; refer PlayerInfo/PlayerSongRecord)
 */
class Player {
protected:
	PlayerPlayConfig*	playconfig;
	int					playside;
	int					playmode;
	int					playertype;

	double				notespeed;			// current note speed
	double				notefloat;			// current note float speed (note visible time; dropping time from lane, strictly.)
	double				speed_mul;			// speed multiplicator (for MAX/MINBPM)
	double				suddenheight;		// 0 ~ 1
	double				liftheight;			// 0 ~ 1

	// some prefetched(pointer) timers/values
	Timer*				pBmstimer;
	PlayerRenderValue*	pv;					// general current player
	PlayerRenderValue*	pv_dp;				// in case of DP

	// guage
	double				playergauge;
	int					playergaugetype;
	bool				dieonnohealth;		// should I die when there's no health?
	double				notehealth[6];		// health up/down per note (good, great, pgreat)

	// DON'T CHEAT! check for value malpulation.
	// (TODO) processed by CryptManager
#ifdef _CRYPTMANAGER
#endif

	// current judge/bar/channel related information
	int					noteindex[20];					// currently processing note index(bar index)
	int					longnotestartpos[20];			// longnote start pos(bar index)
	std::vector<BmsWord> keysound[20];					// sounding key value (per bar index)
	double				health;							// player's health

	// note/time information
	PlayerScore			score;
	BmsNoteManager*		bmsnote;
	int					judgenotecnt;		// judged note count; used for OnLastNote
	Uint32				currenttime;
	Uint32				currentbar;

	bool				IsLongNote(int notechannel);	// are you pressing longnote currently?

	// judge
	int CheckJudgeByTiming(double delta);
	// active note searcher
	int GetCurrentNoteBar(int channel);
	bool IsNoteAvailable(int notechannel);
	int GetAvailableNoteIndex(int notechannel, int start = 0);
	int GetNextAvailableNoteIndex(int notechannel);
	BmsNote GetCurrentSoundNote(int notechannel);		// TODO
	BmsNote* GetCurrentNote(int notechannel);
	/** @brief make judgement. silent = true will not set JUDGE timer. */
	void MakeJudge(int judgetype, int channel, bool silent = false);
public:
	/*
	 * @description
	 * playside: main player is always 0. 1 is available when BATTLE mode.
	 *           if player is flipped, then you should enter value 1.
	 * playmode: SINGLE or DOUBLE or BATTLE? (check PLAYTYPE)
	 * playertype: NORMAL or AUTO or what? (check PLAYERTYPE)
	 */
	Player(PlayerPlayConfig* config, BmsNoteManager *note, int playside = 0,
		int playmode = PLAYTYPE::KEY7, 
		int playertype = PLAYERTYPE::NORMAL);

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
	void SetGauge(double gauge);
	/** @brief Set note speed as normal speed */
	void SetSpeed(double speed);
	void DeltaSpeed(double speed);
	/** @brief Set note speed as float speed */
	void SetFloatSpeed(double speed);
	void DeltaFloatSpeed(double speed);
	/** @brief Set sudden (0 ~ 1) */
	void SetSudden(double height);
	void DeltaSudden(double height);
	/** @brief Set lift (0 ~ 1) */
	void SetLift(double height);
	void DeltaLift(double height);
	/** @brief is player dead? */
	bool IsDead();
};

/*
 * @description
 * Played by CPU
 * If this player activated, 'Autoplay' mark will show up.
 */
class PlayerAuto : public Player {
	double targetrate;

public:
	PlayerAuto(PlayerPlayConfig *config, BmsNoteManager *note,
		int playside = 0, int playmode = PLAYTYPE::KEY7);

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
	PlayerGhost(PlayerPlayConfig *config, BmsNoteManager *note,
		int playside = 0, int playmode = PLAYTYPE::KEY7);
};


/*
 * @description
 * controls player variables & system variables(pool)
 * by using this namespace
 */
namespace PlayHelper {
	/** @description ? */
	void FlipPlayside();		// TODO
	void SpeedChange(int playerno, int delta);
	void LaneChange(int playerno, int delta);
	int LiftChange(int playerno, int delta);

	void ToggleSpeedtype(int playerno);
}

// global-accessible
// real object used during playing game
extern Player*		PLAYER[4];