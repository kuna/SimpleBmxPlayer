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

typedef uint32_t Uint32;
class SkinPlayObject;

/*
 * class Grade
 * - stores current play's score and evaluate
 *
 */
namespace JUDGETYPE {
	const int JUDGE_PGREAT	= 5;
	const int JUDGE_GREAT	= 4;
	const int JUDGE_GOOD	= 3;
	const int JUDGE_POOR	= 2;
	const int JUDGE_EMPTYPOOR	= 2;	// TODO
	const int JUDGE_BAD		= 1;
	const int JUDGE_EARLY	= 10;	// it's too early, so it should have no effect
	const int JUDGE_LATE	= 11;	// it's too late, so it should have no effect
}

namespace GRADETYPE {
	const int GRADE_AAA	= 8;
	const int GRADE_AA	= 7;
	const int GRADE_A	= 6;
	const int GRADE_B	= 5;
	const int GRADE_C	= 4;
	const int GRADE_D	= 3;
	const int GRADE_E	= 2;
	const int GRADE_F	= 1;
}

class Grade {
private:
	int grade[6];
	int notecnt;
	int combo;
	int maxcombo;
public:
	Grade(int notecnt);
	Grade();
	int CalculateScore();
	double CalculateRate();
	int CalculateGrade();
	void AddGrade(const int type);
};

/*
 * class PlayerRecord
 * have data about player's song play records with sqlite
 * (TODO)
 */
class PlayerRecord {
private:
	std::map<BmsInfo, PlayRecord> playrecord;
public:
	void LoadPlayRecord(std::wstring& playerid);
};

// each key/play setting is different!
class PlayerSetting {
public:
	// note/guage/keysetting
	int keysetting[20][4];
	int guageoption;
	int noteoption;

	// speed information
	double speed;
	double lane;
	double lift;
public:
	// type0: SP, type1: DP, type3: 5Key, type4: 10key, type5: PMS
	void LoadPlaySetting(const char* path);
	void SavePlaySetting(const char* path);
};

namespace PLAYERTYPE {
	const int NORMAL = 0;
	const int AUTO = 1;
	const int REPLAY = 2;
	const int NETWORK = 3;	// not implemented
}

/*
 * @description
 * Very basic form of Player
 * Generally used for playing
 */
class Player {
protected:
	// stored/storing settings
	// COMMENT: we don't need laneheight.
	// we just need to get relative position of lane(0 ~ 1). 
	// skinelement will process it by itself.
	PlayerSetting		setting;
	int					playside;
	int					playertype;
	double				speed;				// for calculating speed_mul
	double				speed_mul;			// absbeat * speed_mul = (real y pos)
	int					speed_type;			// TODO

	// some prefetch timers
	Timer*				bmstimer;			// elapsed time
	Timer*				misstimer;			// timer used when miss occured
	Timer*				lanepress[20];
	Timer*				lanehold[20];
	Timer*				laneup[20];
	Timer*				lanejudgeokay[20];
	Image*				currentmissbga;		// when miss occurs, get current Miss BGA

	// grade information
	Grade				grade;

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
	Grade GetGrade();
	int GetCurrentBar();
	int GetCurrentNoteBar(int channel);
	bool IsNoteAvailable(int notechannel);
	int GetAvailableNoteIndex(int notechannel, int start = 0);
	bool IsDead();
	bool IsFinished();	// TODO
	void SetPlayerSetting(const PlayerSetting& setting);
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

}