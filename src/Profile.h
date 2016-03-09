/*
 * @description
 * stores all information player info/customize
 * - basic information: ID, PASS, NAME, grade?
 * - key configs
 * - song clear status
 */

#pragma once
#include "global.h"
#include "tinyxml2.h"
#include <vector>




class PlayOption {
public:
	int randomS1, randomS2;	// S2 works for double play
	double freq;			// needless to say?
	int longnote;			// off, legacy, 20%, 50%, 100%
	int morenote;			// off, -50%, -20%, 20%, 50%
	int judge;				// off, extend, hard, vhard
	int scratch;			// off, assist, all_sc
	int rseed;				// random seed (-1: random)
	int flip;

	PlayOption();
	void DefaultOption();
	bool ParseOptionString(const RString& option);
	RString GetOptionString(const RString& option);
};

struct PlayConfig {
	double sudden, lift;
	int showsudden, showlift;

	int ghost_type;			// 
	int judge_type;			// fastslow
	int pacemaker_type;		// 0%, 90%, 100%, rival, mybest, A, AA, AAA, custom
	int pacemaker_goal;		// custom goal

	double speed;
	int speedtype;			// off, float, max, min, medium, constant(assist)
	double floatspeed;		// float speed itself
	int usefloatspeed;		// should we use float speed?
	int gaugetype;			// off, assist, ...

	// judge offset
	int judgeoffset;
	int judgecalibration;
};

class PlayScore {
public:
	int score[6];
	int totalnote;
	int combo;
	int maxcombo;
	int slow, fast;
public:
	PlayScore(int notecnt);
	PlayScore();

	// utils
	void Clear();
	int LastNoteFinished() const;
	int GetJudgedNote() const;
	double CurrentRate() const;
	int CalculateEXScore() const;
	int CalculateScore() const;
	double CalculateRate() const;
	int CalculateGrade() const;
	void AddGrade(const int type);
	void AddSlow() { slow++; }
	void AddFast() { fast++; }
};

/*
 * @description record for each song data
 */
class PlayRecord {
public:
	// used for identifying song
	RString hash;
	// game play options
	RString playoption;		// decides note chart
	int type;				// kb? beatcon? screen? midi?
	// game play statics
	int playcount;
	int clearcount;
	int failcount;
	int clear;				// cleared status
	// calculated from game play record
	int minbp;
	int cbrk;
	int maxcombo;			// CAUTION: it's different fram grade.
	// records used for game play
	PlayScore score;		// we only store high-score
public:
	// used for hashing (scorehash)
	RString Serialize();
};





struct ReplayEvent {
	unsigned int time;
	/*
	* single side: 0xAF
	* double side: 0xBF
	*/
	unsigned int lane;
	/*
	* note press: 1, up: 0
	* if judge: 0 ~ 5
	* fast: +0x0010 (16)
	* slow: +0x0020 (32)
	* silent: +0x0100 (256)
	*/
	unsigned int value;

	bool IsJudge();
	int GetSide();
	int GetJudge();
	int GetFastSlow();
	int GetSilent();
};

class ReplayData {
private:
	// about chart
	int op_1p, op_2p;
	int gauge;		// TODO: change it into string
	int rseed;
	double rate;
	// store judge for each note
	std::vector<ReplayEvent> objects[10];
	int round = 0;	// current round status
public:
	typedef std::vector<ReplayEvent>::iterator Iterator;
	Iterator Begin() { return objects[round].begin(); }
	Iterator End() { return objects[round].end(); }
	void Clear();
	void Serialize(RString& out) const;		// Get base64 zipped string
	void Parse(const RString& in);			// Input base64 zipped string
	void SetRound(int v);

	void AddPress(int time, int lane, int press);
	void AddJudge(int time, int playside, int judge, int fastslow, int silent = 0);
};



#include "Theme.h"

class Profile {
public:
	// basic informations
	RString name;
	int spgrade;
	int dpgrade;
	int playcount;					// total record
	int failcount;
	int clearcount;
	// select option
	PlayScore score;				// total record
	PlayConfig playconfig;			// last play option
	bool isloaded;

protected:
	// theme metrics
	Value<RString>	sPlayerName;
	Value<int>		sPlayerLevel;
	Value<RString>	iPlayerScorePerfect;
	Value<RString>	iPlayerScoreGreat;
	Value<RString>	iPlayerScoreGood;
	Value<RString>	iPlayerScoreBad;
	Value<RString>	iPlayerScorePoor;

	Value<int>		iRecordClear;
	Value<int>		iRecordScore;
	Value<int>		iRecordCombo;
	Value<int>		iReplayExists;
public:
	Profile(int side);
	~Profile();

	void DefaultProfile(const RString& name);	// initialize profile setting with name.
	bool LoadProfile(const RString& name);		// returns false if it's new profile.
	void UnloadProfile();
	void SaveProfile(const RString& name);
	bool IsProfileLoaded();						// check if this profile slot is loaded.

	void IsSongRecordExists(const RString& songhash);
	bool LoadSongRecord(const RString& songhash, PlayRecord &rec);						// returns false if it's not existing record
	void SaveSongRecord(const RString& songhash, PlayRecord &rec);
	void DeleteSongRecord(const RString& songhash);

	bool IsReplayDataExists(const RString& songhash);
	bool LoadReplayData(const RString& songhash, ReplayData &rep);						// returns false if it's not existing replay
	void SaveReplayData(const RString& songhash, ReplayData &rep);

	void SetPlayOptionMetrics();
	void SetPlayScoreMetrics();
	void SetReplayExistsMetrics(bool v);
};


// PROFILE SLOT, globally accessible.
extern Profile*		PROFILE[2];