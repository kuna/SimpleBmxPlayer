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






// pure player related

struct PlayerPlayConfig {
	// playing option
	int op_1p, op_2p;
	double sudden, lift;
	int showsudden, showlift;
	double speed;
	int speedtype;		// off, float, max, min, medium, constant(assist)
	double floatspeed;	// float speed itself
	int usefloatspeed;	// should we use float speed?
	int gaugetype;		// off, assist, ...
	// pacemaker
	int ghost_type;		// 
	int judge_type;		// fastslow
	int pacemaker_type;	// 0%, 90%, 100%, rival, mybest, A, AA, AAA, custom
	int pacemaker_goal;	// custom goal
	// assists/additional option
	int longnote;		// off, legacy, 20%, 50%, 100%
	int morenote;		// off, -50%, -20%, 20%, 50%
	int judge;			// off, extend, hard, vhard
	int scratch;		// off, assist, all_sc
	double freq;		// needless to say?
	// judge offset
	int judgeoffset;
	int judgecalibration;
};

class PlayerInfo {
public:
	// basic informations
	RString name;
	int spgrade;
	int dpgrade;
	int playcount;					// total record
	int failcount;
	int clearcount;
	// select option
	PlayerScore score;				// total record
	PlayerPlayConfig playconfig;	// last play option
public:
	bool LoadPlayerSetting(const RString& name);
	bool SavePlayerSetting(const RString& name);
	/** @brief returns SongRecord object from sqlite db. returns false if not found. */
	/** @brief saves SongRecord object to sqlite db. */
};

namespace PlayerInfoHelper {
	/** @brief set playerinfo as default (COMMENT: don't care playername) */
	void DefaultPlayerInfo(PlayerInfo& player);
	bool LoadPlayerInfo(PlayerInfo& player, const char* playername);
	bool SavePlayerInfo(PlayerInfo& player);
}

namespace PlayOptionHelper {
	void DefaultPlayConfig(PlayerPlayConfig &config);
	void LoadPlayConfig(PlayerPlayConfig &config, tinyxml2::XMLNode *base);
	void SavePlayConfig(const PlayerPlayConfig &config, tinyxml2::XMLNode *base);
}

// global variables game player
// Beatmania only could have 2 players, 
// so we don't need to take care of more players.
extern PlayerInfo		PLAYERINFO[2];







// song record related

class PlayerScore {
public:
	int score[6];
	int totalnote;
	int combo;
	int maxcombo;
	int slow, fast;
public:
	PlayerScore(int notecnt);
	PlayerScore();

	// just a utils
	void Clear();
	int LastNoteFinished() const;
	int GetJudgedNote() const;
	double CurrentRate() const;
	int CalculateEXScore() const;
	int CalculateScore() const;
	double CalculateRate() const;
	int CalculateGrade() const;
	void AddGrade(const int type);
	int Slow() { slow++; }
	int Fast() { fast++; }
};

/*
 * @description record for each song data
 */
class PlayerSongRecord {
public:
	// used for identifying song
	RString hash;
	// game play options
	int op1, op2;			// decides note chart
	int rseed;				// decides note chart
	int type;				// kb? beatcon?
	// game play statics
	int playcount;
	int clearcount;
	int failcount;
	int status;				// cleared status
	// calculated from game play record
	int minbp;
	int maxcombo;			// CAUTION: it's different fram grade.
	// records used for game play
	PlayerScore score;		// we only store high-score
public:
	// used for hashing (scorehash)
	RString Serialize();
};

namespace PlayerRecordHelper {
	bool LoadPlayerRecord(PlayerSongRecord& record, const char* playername, const char* songhash);
	bool SavePlayerRecord(const PlayerSongRecord& record, const char* playername);
	bool DeletePlayerRecord(const char* playername, const char* songhash);
}







// replay related

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

namespace ReplayHelper {
	bool LoadReplay(ReplayData& rep, const char* playername, const char* songhash, const char* course = 0);
	bool SaveReplay(const ReplayData& rep, const char* playername, const char* songhash, const char* course = 0);
}