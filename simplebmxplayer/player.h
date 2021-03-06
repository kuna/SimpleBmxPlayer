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
* @describes
* stores all variables about player skin rendering value
* initalized at ScenePlay::Initialize()
*/
typedef struct {
	Timer*				pOnMiss;			// timer used when miss occured (DP)
	Timer*				pOnCombo;
	Timer*				pOnJudge[6];		// pf/gr/gd/bd/pr
	Timer*				pOnSlow;
	Timer*				pOnFast;
	Timer*				pOnfullcombo;		// needless to say?
	Timer*				pOnlastnote;		// when last note ends
	Timer*				pOnGameover;		// game is over! (different from OnClose)
	Timer*				pOnGaugeMax;		// guage max?
	Timer*				pOnGaugeUp;
	Timer*				pLanepress[10];
	Timer*				pLanehold[10];
	Timer*				pLaneup[10];
	Timer*				pLanejudgeokay[10];
	int*				pNotePerfect;
	int*				pNoteGreat;
	int*				pNoteGood;
	int*				pNoteBad;
	int*				pNotePoor;
	Timer*				pOnAAA;
	Timer*				pOnAA;
	Timer*				pOnA;
	Timer*				pOnB;
	Timer*				pOnC;
	Timer*				pOnD;
	Timer*				pOnE;
	Timer*				pOnF;
	Timer*				pOnReachAAA;
	Timer*				pOnReachAA;
	Timer*				pOnReachA;
	Timer*				pOnReachB;
	Timer*				pOnReachC;
	Timer*				pOnReachD;
	Timer*				pOnReachE;
	Timer*				pOnReachF;
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
	int*				pSudden;
	int*				pLift;
	double*				pSudden_d;
	double*				pLift_d;
} PlayerRenderValue;

/*
 * @description
 * Very basic form of Player
 * only stores data for real game play (not rendering)
 * (COMMENT: not storing data for each player; refer PlayerInfo/PlayerSongRecord)
 */
class Player {
protected:
	PlayerPlayConfig*	playconfig;
	PlayerReplayRecord	playrecord;
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
	Timer*				pLanepress[20];
	Timer*				pLaneup[20];
	Timer*				pLanehold[20];
	Timer*				pLanejudgeokay[20];

	// guage
	double				playergauge;
	int					playergaugetype;
	bool				dieonnohealth;		// should I die when there's no health?
	double				notehealth[6];		// health up/down per note (good, great, pgreat)

	// current judge/bar/channel related information
	double					health;						// player's health
	int						*combo;						// combo (currently displaying) TODO

	// note/time information
	PlayerScore				score;
	BmsNoteManager*			bmsnote;
	BmsNoteLane::Iterator	iter_judge_[20];			// current judging iterator
	BmsNoteLane::Iterator	iter_end_[20];				// 
	BmsNoteLane::Iterator	iter_begin_[20];			// 
	bool					islongnote_[20];			// currently longnote pressing?
	bool					ispress_[20];				// currently pressing?

	BmsNote*				GetCurrentNote(int lane) {
		if (iter_judge_[lane] == iter_end_[lane]) return 0;
		else return &iter_judge_[lane]->second;
	};

	// judge
	/** @brief judge timing offset (LR2's judge offset) */
	int						judgeoffset;
	int						judgecalibration;
	int						CheckJudgeByTiming(int delta);
	/** @brief make judgement. silent = true will not set JUDGE timer. */
	void					MakeJudge(int delta, int time, int channel, int fastslow = 0, bool silent = false);
	/** @brief is there any more note to draw/judge? */
	bool					IsNoteAvailable(int lane);
	void					NextNote(int lane);
	
	/* internal function */
	void					PlaySound(BmsWord& value);
public:
	void					InitalizeNote();			// create note data (must called after bms loaded)
	void					InitalizeScore();
	void					InitalizeGauge();

	BmsNoteLane::Iterator	GetNoteIter(int lane) { return iter_judge_[lane]; };
	BmsNoteLane::Iterator	GetNoteEndIter(int lane) { return iter_end_[lane]; };
	BmsNoteLane::Iterator	GetNoteBeginIter(int lane) { return iter_begin_[lane]; };
	BmsNoteManager*			GetNoteData() { return bmsnote; };
	PlayerScore*			GetScoreData() { return &score; };

	// etc
	bool					issilent;		// only score, don't play sound / graph / timer.

	/*
	 * @description
	 * playside: main player is always 0. 1 is available when BATTLE mode.
	 *           if player is flipped, then you should enter value 1.
	 * playmode: SINGLE or DOUBLE or BATTLE? (check PLAYTYPE)
	 * playertype: NORMAL or AUTO or what? (check PLAYERTYPE)
	 */
	Player(int playside = 0, int playmode = PLAYTYPE::KEY7, 
		int playertype = PLAYERTYPE::NORMAL);
	~Player();

	/*
	 * @brief 
	 * Update late note/note position
	 * Uses Global Game(BmsResource) Timer
	 */
	virtual void Update();
	/* 
	 * @brief update basics, like combo / miss / rendering note iterator / etc.
	 */
	void UpdateBasic();

	/*
	 * @brief
	 * Hard-Reset player's note position
	 * Not generally used, only used for starting from specific position.
	 */
	virtual void Reset(barindex bar);

	/** @brief General key input for judgement */
	virtual void UpKey(int keychannel);
	/** @brief General key input for judgement */
	virtual void PressKey(int keychannel);

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
	double GetSpeedMul();
	/** @brief is player dead? */
	bool IsDead();

	/** @brief should play sound? (related to pacemaker) */
	void Silent(bool silent = true);
};

/*
 * @description
 * Played by CPU
 * If this player activated, 'Autoplay' mark should be show up.
 */
class PlayerAuto : public Player {
	double targetrate;

public:
	PlayerAuto(int playside = 0, int playmode = PLAYTYPE::KEY7);

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
class PlayerReplay : public Player {
public:
	PlayerReplay(int playside = 0, int playmode = PLAYTYPE::KEY7);

	void SetReplay(const PlayerReplayRecord &rep);
	virtual void Update();
	virtual void PressKey(int channel);
	virtual void UpKey(int channel);

private:
	PlayerReplayRecord::Iterator iter_;
	PlayerReplayRecord replay;
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
extern PlayerRenderValue	PLAYERVALUE[4];
extern Player*				PLAYER[4];