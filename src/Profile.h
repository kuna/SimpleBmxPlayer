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




/*
 * playing option, which has close relation with game playing (command enabled) 
 * different from MOD command (that's only used for playing, and not saved to original option)
 *
 * Training mode (repeat, start, end), BGA: BmsHelper (loader)
 * Playrate: SongPlayer
 */
class PlayOption {
public:
	int randomS1, randomS2;	// S2 works for double play
	int longnote;			// off, legacy, 20%, 50%, 100%
	int morenote;			// off, -50%, -20%, 20%, 50%
	int scratch;			// off, assist, all_sc
	int flip;

	int gaugetype;			// off, assist, ...
	int gaugeval[6];		// (not saved, custom gauge)
	int judgetype;			// off, extend, hard, vhard
	int judgeval[6];		// (not saved, custom gauge)

	double sudden, lift;
	int showsudden, showlift;
	double speed;
	int speedtype;			// off, float, max, min, medium, constant(assist)
	double floatspeed;		// float speed itself
	int usefloatspeed;		// should we use float speed?
	/*
	 * COMMENT:
	 * float speed isn't main speed - that is, used only for reference.
	 * float speed is only refreshed when `speed change started`.
	 */

	PlayOption();
	void DefaultOption();
	void ParseOptionString(const RString& option);
	RString GetOptionString();
	bool IsAssisted();
};

/* customized setting for player (static during play) */
struct PlayConfig {
	PlayConfig();
	int ghost_type;			// 
	int judge_type;			// fastslow
	int pacemaker_type;		// 0%, 90%, 100%, rival, mybest, A, AA, AAA, custom
	int pacemaker_goal;		// custom goal
	// judge offset
	int judgeoffset;
	int judgecalibration;
};

class PlayScore {
public:
	int score[6];
	int totalnote;
	int combo, cbrk, maxcombo;
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
	// store judge for each note
	RString m_Cmd;
	RString m_Songhash;
	std::vector<ReplayEvent> objects[10];
	int round = 0;	// current round status
public:
	typedef std::vector<ReplayEvent>::iterator Iterator;
	Iterator Begin() { return objects[round].begin(); }
	Iterator End() { return objects[round].end(); }
	void SetMetadata(const RString& cmd, const RString& songhash);
	RString GetCmd() { return m_Cmd; }
	RString GetSonghash() { return m_Songhash; }
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
	int playcount;				// total record
	int failcount;
	int clearcount;
	// select option
	PlayScore score;			// total record
	PlayConfig config;			// play setting
	PlayOption option;			// play option used for game playing
	int playertype;				// off, human, network

protected:
	// theme metrics
	Value<RString>	sPlayerName;
	Value<int>		iPlayerLevel;
	Value<int>		iPlayerSpGrade;
	Value<int>		iPlayerDpGrade;
	Value<int>		iPlayerScorePerfect;
	Value<int>		iPlayerScoreGreat;
	Value<int>		iPlayerScoreGood;
	Value<int>		iPlayerScoreBad;
	Value<int>		iPlayerScorePoor;

	Value<int>		iPlayerGauge;
	Value<int>		iPlayerRandomS1;
	Value<int>		iPlayerRandomS2;
	Value<int>		iPlayerScratch;
	Value<int>		iPlayerFlip;
	Value<int>		iPlayerBattle;
	Value<int>		iPlayerSpeed;
	Value<int>		iPlayerSpeedtype;
	Value<int>		iPlayerRange;		// sudden, lift, ...
	Value<int>		iPlayerMode;

	Value<int>		iPlayerPacemaker;
	Value<int>		iPlayerPacemakerGoal;
	Value<int>		iPlayerJudgetiming;
	Value<int>		iPlayerJudgeoffset;
	SwitchValue		isPlayerGhostPosition[4];
	SwitchValue		isPlayerJudgePosition[4];
	Value<int>		iPlayerScoreGraph;
	SwitchValue		isPlayerScoreGraph;

	Value<int>		iReplayExists;
	Value<int>		iRecordClearcount;
	Value<int>		iRecordFailcount;
	Value<int>		iRecordClear;
	Value<int>		iRecordScore[6];
	Value<int>		iRecordExScore;
	Value<int>		iRecordRate;
	Value<double>	iRecordRate_d;
	Value<int>		iRecordRank;
	Value<int>		iRecordCombo;
	Value<int>		iRecordMinBP;
	Value<int>		iRecordCBRK;
	Value<int>		iRecordMaxCombo;

	void UpdateInfoMetrics();
	void UpdateConfigMetrics();
	void UpdateOptionMetrics();
public:
	Profile(int side);
	~Profile();

	void DefaultProfile(const RString& name);	// initialize profile setting with name.
	bool LoadProfile(const RString& name);		// returns false if it's new profile.
	void UnloadProfile();
	void SaveProfile(const RString& name);
	bool IsProfileLoaded();						// check if this profile slot is loaded.

	bool IsSongRecordExists(const RString& songhash);
	bool LoadSongRecord(const RString& songhash, PlayRecord &rec);						// returns false if it's not existing record
	void SaveSongRecord(const RString& songhash, PlayRecord &rec);
	void DeleteSongRecord(const RString& songhash);

	bool IsReplayDataExists(const RString& songhash);
	bool LoadReplayData(const RString& songhash, ReplayData &rep);						// returns false if it's not existing replay
	void SaveReplayData(const RString& songhash, ReplayData &rep);

	// set from external settings
	void SetPlayConfig(const PlayConfig& config);
	void SetPlayOption(const RString& cmd);
	PlayOption* GetPlayOption() { return &option; }
	PlayConfig* GetPlayConfig() { return &config; }
};


// PROFILE SLOT, globally accessible.
extern Profile*		PROFILE[2];