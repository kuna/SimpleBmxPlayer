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
#include "Profile.h"
#include "Theme.h"
#include "global.h"




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
	BmsNote* Seek();						// seek next note object
	BmsNote* GetSoundNote();				// returns 0 if no soundable note
	void Next();
	bool IsEndOfNote();
};

struct Judge {
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
	Profile*			m_Profile;
	PlayOption			m_Option;

	// playing settings
	int					playside;
	int					playmode;			// used for DP keyinput supporting
	int					playertype;
	int					seed;

	double				note_total;			// gauge per note
	double				speed_mul;			// speed multiplicator (for MAX/MINBPM)

	double				health;
	double				gaugeval[6];		// health up/down per note (good, great, pgreat)
	int					judgeval[6];		// judgement delta time

	// record disabled in case of training mode / replay
	bool m_IsRecordable;
	bool m_Giveup = false;
	bool m_PlaySound;			// decide to play key sound (if pacemaker, then make this false)
	bool m_Dieonnohealth;		// should I die when there's no health?

	// note/time information
	BmsNoteManager*		bmsnote;			// Don't store total bms object, only store note object
	Lane				m_Lane[20];
	uint32_t			m_BmsTime;
	PlayScore			m_Score;
	ReplayData			m_ReplayRec;

	/* check, make judge */
	int					judgeoffset;
	int					judgecalibration;
	/*
	 * Each note should have it's own judge.
	 * That is, you should make UpdateJudge() to iterate over notes
	 */
	int					GetJudgement(int delta);
	bool				AddJudgeDelta(int channel, int delta, bool silent = false);
	bool				AddJudge(int channel, int judge, int fastslow, bool silent = false);
	Judge				m_curJudge;

	/* Theme metrics (only needed during playing) */
	SwitchValue			pOnfullcombo;		// needless to say?
	SwitchValue			pOnlastnote;		// when last note ends
	SwitchValue			pOnGameover;		// game is over! (different from OnClose)
	SwitchValue			pOnGaugeMax;		// guage max?
	SwitchValue			pOnGaugeUp;
	SwitchValue			OnOptionChange;		// pressing start button during play
	SwitchValue			pOnRank[8];			// F ~ AAA
	SwitchValue			pOnReachRank[8];	// F ~ AAA
	Value<double>		pRate_d;
	Value<int>			pRate;
	Value<double>		pTotalRate_d;
	Value<int>			pTotalRate;

	Value<int>			pNotePerfect;
	Value<int>			pNoteGreat;
	Value<int>			pNoteGood;
	Value<int>			pNoteBad;
	Value<int>			pNotePoor;
	SwitchValue			pOnJudge[6];		// pf/gr/gd/bd/pr
	SwitchValue			pOnSlow;
	SwitchValue			pOnFast;
	SwitchValue			pOnMiss;			// Switch used when miss occured (DP)
	SwitchValue			pOnCombo;
	Value<int>			pTotalnotes;
	Value<int>			pCombo;
	Value<int>			pMaxCombo;
	Value<int>			pScore;
	Value<int>			pExscore;
	Value<double>		pExscore_d;
	Value<double>		pHighscore_d;

	Value<int>			pGaugeType;
	Value<double>		pGauge_d;
	Value<int>			pGauge;
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

	void UpdateSpeedMetric();

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

	void				Clear();						// MUST call this function after profile/note is set.
	void				SetProfile(Profile* p);
	void				SetNote(BmsBms* bms);			// create note data (must called after bms loaded)
	void				SetKey(int keycount);			// Must set to play properly in DP

	void				SetPlaySound(bool v) { m_PlaySound = v; }
	int					GetPlayerType() { return playertype; }

	/* used for rendering notes */
	BmsNoteManager*		GetNoteData() { return bmsnote; };
	/* used for saving score */
	PlayScore*			GetScoreData() { return &m_Score; };
	/* used for saving replay */
	ReplayData*			GetRecordData() { return &m_ReplayRec; }


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
	void SetGauge(double gauge);	// TODO ...?
	void RefreshFloatSpeed();
	void SetSpeed(double speed);
	void DeltaSpeed(double speed);
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
// - PLAYER[0/1] : main player
// - PLAYER[2]   : MYBEST (replay object)
extern Player*				PLAYER[4];


// or, 
//
// PLAYER[2]
// PLAYER_PACEMAKER[2]
// PLAYER_MYBEST
// - like this?
//