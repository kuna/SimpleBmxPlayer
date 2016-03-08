/*
 * Manages playing state of a player.
 * Setup value: Bms object, speed.
 * Input value: time, keypress, keyup.
 *
 * player type: Normal(Basic) / CPU(Auto) / Replay
 * player contains: BGA status / Combo / Score Rate / Health / Note / Lain Lift / Speed
 */

#pragma once

#include "bmsbel/bms_bms.h"
#include "Pool.h"
#include "global.h"
#include "playerinfo.h"




struct Lane {
	int idx;
	BmsNoteLane::Iterator iter_judge;
	BmsNoteLane::Iterator iter_end;
	BmsNoteLane::Iterator iter_begin;		// used when find pressing keysound (iter >= 0)
	SwitchValue pLanePress;
	SwitchValue pLaneHold;
	SwitchValue pLaneUp;
	SwitchValue pLaneOkay;

	bool IsPressing();
	bool IsLongNote();
	BmsNote* GetCurrentNote();
	BmsNote* GetSoundNote();				// returns 0 if no soundable note
	void Next();
	bool IsEndOfNote();
};

struct Judge {
	int delta;
	int lane;
	int fastslow;
	bool silent;
};




/*
 * @description
 * Very basic form of Player
 * only stores data for real game play (not rendering)
 * (COMMENT: not storing data for each player; refer PlayerInfo/PlayerSongRecord)
 */
class Player {
protected:
	// player's basic information (OP / RSEED ...)
	PlayerPlayConfig*		playconfig;
	/*
	int op1;			// 0x0000ABCD; RANDOM / SC / LEGACY(MORENOTE/ALL-LN) / JUDGE
	int op2;
	int rseed;
	*/

	// playing settings
	int					playside;
	int					playmode;			// used for DP keyinput supporting
	int					playertype;

	double				notespeed;			// current note speed
	double				notefloat;			// current note float speed (note visible time; dropping time from lane, strictly.)
	double				speed_mul;			// speed multiplicator (for MAX/MINBPM)
	double				suddenheight;		// 0 ~ 1
	double				liftheight;			// 0 ~ 1

	double				playergauge;
	int					playergaugetype;
	bool				dieonnohealth;		// should I die when there's no health?
	double				notehealth[6];		// health up/down per note (good, great, pgreat)
	double				health;				// player's health
	bool				m_PlaySound;		// decide to play key sound (if pacemaker, then make this false)

	// record disabled in case of training mode / replay
	bool m_IsRecordable;
	bool m_Giveup = false;

	// note/time information
	uint32_t			m_BmsTime;
	PlayerScore			score;
	ReplayData			replay_cur;
	BmsNoteManager*		bmsnote;			// Don't store total bms object, only store note object
	Lane				m_Lane[20];

	/* check, make judge */
	int					judgeoffset;
	int					judgecalibration;
	int					CheckJudgeByTiming(int delta);
	/*
	 * Each note should have it's own judge.
	 * That is, you should make UpdateJudge() to iterate over notes
	 */
	bool				UpdateJudge(int objtime, int channel, bool silent = false);
	void				AddJudge(int judge, int channel, int fastslow, bool silent = false);
	Judge				m_curJudge;

	/* Theme metrics (only needed during playing) */
	SwitchValue			pOnMiss;			// Switch used when miss occured (DP)
	SwitchValue			pOnCombo;
	SwitchValue			pOnJudge[6];		// pf/gr/gd/bd/pr
	SwitchValue			pOnSlow;
	SwitchValue			pOnFast;
	SwitchValue			pOnfullcombo;		// needless to say?
	SwitchValue			pOnlastnote;		// when last note ends
	SwitchValue			pOnGameover;		// game is over! (different from OnClose)
	SwitchValue			pOnGaugeMax;		// guage max?
	SwitchValue			pOnGaugeUp;
	Value<int>			pNotePerfect;
	Value<int>			pNoteGreat;
	Value<int>			pNoteGood;
	Value<int>			pNoteBad;
	Value<int>			pNotePoor;
	SwitchValue			pOnAAA;
	SwitchValue			pOnAA;
	SwitchValue			pOnA;
	SwitchValue			pOnB;
	SwitchValue			pOnC;
	SwitchValue			pOnD;
	SwitchValue			pOnE;
	SwitchValue			pOnF;
	SwitchValue			pOnReachAAA;
	SwitchValue			pOnReachAA;
	SwitchValue			pOnReachA;
	SwitchValue			pOnReachB;
	SwitchValue			pOnReachC;
	SwitchValue			pOnReachD;
	SwitchValue			pOnReachE;
	SwitchValue			pOnReachF;
	SwitchValue			pOnMiss;
	Value<double>		pExscore_d;
	Value<double>		pHighscore_d;
	Value<int>			pScore;
	Value<int>			pExscore;
	Value<int>			pCombo;
	Value<int>			pMaxCombo;
	Value<int>			pTotalnotes;
	Value<int>			pRivaldiff;		// TODO where to process it?
	Value<double>		pGauge_d;
	Value<int>			pGaugeType;
	Value<int>			pGauge;
	Value<double>		pRate_d;
	Value<int>			pRate;
	Value<double>		pTotalRate_d;
	Value<int>			pTotalRate;
	Value<int>			pNoteSpeed;
	Value<int>			pFloatSpeed;
	Value<int>			pSudden;
	Value<int>			pLift;
	Value<double>		pSudden_d;
	Value<double>		pLift_d;
	SwitchValue			pAutoplay;
	SwitchValue			pReplay;
	SwitchValue			pHuman;
	SwitchValue			pNetwork;

	/* 
	 * @brief update basics, like combo / miss / rendering note iterator / etc.
	 * (internal function)
	 */
	void UpdateBasic();
	/* internal function */
	void PlaySound(BmsWord& value);
public:
	Player(int playside = 0, int playertype = PLAYERTYPE::HUMAN);
	~Player();

	void				SetKey(int keycount);			// Must set to play properly in DP
	void				InitalizeNote(BmsBms* bms);		// create note data (must called after bms loaded)
	void				InitalizeGauge();				// initalize gauge - not called in course mode stage.
	void				SetPlaySound(bool v) { m_PlaySound = v; }
	int					GetPlayerType() { return playertype; }

	/* used for rendering notes */
	BmsNoteManager*		GetNoteData() { return bmsnote; };
	/* used for saving score */
	PlayerScore*		GetScoreData() { return &score; };
	/* used for saving replay */
	ReplayData*			GetRecordData() { return &replay_cur; }


	/*
	 * @brief 
	 * Update late note/note position
	 * Uses Global Game(BmsResource) Switch
	 */
	virtual void Update();

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


	/** @brief save play record & replay data */
	void Save();

	bool IsSaveable();
	bool IsDead();
	bool IsFinished();
	bool IsHuman();
};

/*
 * @description
 * Played by CPU
 * Also available in network versus player.
 */
class PlayerAuto : public Player {
	double targetrate;

public:
	PlayerAuto(int playside = 0);

	virtual void Update();
	virtual void PressKey(int channel);
	virtual void UpKey(int channel);

	void SetGoal(double rate);
	void SetGauge(double gauge);
	void SetAsDead();
	void SetCombo(int combo);
	void BreakCombo();
};

/*
 * @description
 * Play from Replay data.
 */
class PlayerReplay : public Player {
public:
	PlayerReplay(int playside = 0);

	void SetReplay(const ReplayData &rep);
	virtual void Update();
	virtual void PressKey(int channel);
	virtual void UpKey(int channel);

private:
	ReplayData::Iterator iter_;
	ReplayData replay;
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


//
// global-accessible
// As this object is used in Themes / ScenePlay.
//
extern Player*				PLAYER[4];