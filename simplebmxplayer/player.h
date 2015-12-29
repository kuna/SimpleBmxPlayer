#pragma once

#include "bmsbel/bms_bms.h"
#include "bmsinfo.h"
#include "playrecord.h"
#include <stdint.h>		// for Uint32 definition

typedef uint32_t Uint32;

/*
 * class Grade
 * - stores current play's score and evaluate
 *
 */
class Grade {
private:
	int grade[6];
	int notecnt;
	int combo;
	int maxcombo;

public:
	static const int JUDGE_PGREAT;
	static const int JUDGE_GREAT;
	static const int JUDGE_GOOD;
	static const int JUDGE_POOR;
	static const int JUDGE_BAD;
	static const int JUDGE_EARLY;	// it's too early, so it should have no effect
	static const int JUDGE_LATE;	// it's too late, so it should have no effect

	static const int GRADE_AAA;
	static const int GRADE_AA;
	static const int GRADE_A;
	static const int GRADE_B;
	static const int GRADE_C;
	static const int GRADE_D;
	static const int GRADE_E;
	static const int GRADE_F;
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

/*
 * class Player
 * - stores current player(side)'s state
 */
class Player {
private:
	// stored/storing settings
	PlayerSetting setting;
	int playside;

	// grade information
	Grade grade;

	// current judge/bar/channel related information
	int noteindex[20];					// currently processing note index(bar index)
	int longnotestartpos[20];			// longnote start pos(bar index)
	bool IsLongNote(int notechannel);	// are you pressing longnote currently?
	std::vector<BmsWord> keysound[20];	// sounding key value (per bar index)
	int currentbar;
	double health;						// player's health

	// note/time information
	BmsBms *bms;
	BmsTimeManager bmstime;
	BmsNoteContainer bmsnote;
	Uint32 currenttime;
	int keys;

	// autoplay?
	bool autoplay;
	
	// judge
	int CheckJudgeByTiming(double delta);
	// active note searcher
	int GetNextAvailableNoteIndex(int notechannel);
	BmsNote* GetCurrentNote(int notechannel);
public:
	Player();

	// game flow
	void Prepare(BmsBms *bms, int startpos, bool autoplay, int playside);
	void SetTime(Uint32 tick);
	void ResetTime(Uint32 tick);

	// key input
	void UpKey(int keychannel);
	void PressKey(int keychannel);

	// get/set status
	BmsWord GetCurrentMissBga();
	BmsTimeManager& GetBmsTimeManager();
	BmsNoteContainer& GetBmsNotes();
	double GetSpeed();
	Grade GetGrade();
	double GetCurrentPos();
	int GetCurrentBar();
	int GetCurrentNoteBar(int channel);
	bool IsNoteAvailable(int notechannel);
	int GetAvailableNoteIndex(int notechannel, int start = 0);
	bool IsFinished();	// TODO
	void SetPlayerSetting(const PlayerSetting& setting);

	// handler
	void SetOnSound(void(*func)(BmsWord, int));
	void SetOnBga(void(*func)(BmsWord, BmsChannelType::BMSCHANNEL));
	void SetOnJudge(void(*func)(int ,int));
};