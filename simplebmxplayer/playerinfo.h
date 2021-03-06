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

#define _MAX_KEYCONFIG_MATCH	8

struct PlayerKeyConfig {
	int keycode[40][_MAX_KEYCONFIG_MATCH];
};

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
* PlayRecordObject: objects consisting play record
* PlayerReplayRecord: a record of a playing
*/
struct ReplayObject {
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
};

class PlayerReplayRecord {
private:
	// about chart
	RString songhash;
	int op_1p, op_2p;
	int gauge;
	int rseed;
	double rate;
	// store judge for each note
	std::vector<ReplayObject> objects;
public:
	typedef std::vector<ReplayObject>::iterator Iterator;
	Iterator Begin() { return objects.begin(); }
	Iterator End() { return objects.end(); }
	void AddPress(int time, int lane, int press);
	void AddJudge(int time, int playside, int judge, int fastslow, int silent = 0);
	void Clear();
	void Serialize(RString& out) const;		// Get base64 zipped string
	void Parse(const RString& in);			// Input base64 zipped string
};

/*
 * @description record for each song data
 */
class PlayerSongRecord {
public:
	// TODO: add date
	// used for identifying song
	RString hash;
	// records less important for game play 
	int playcount;
	int clearcount;
	int failcount;
	int status;				// cleared status
	// calculated from game play record
	int minbp;
	int maxcombo;			// CAUTION: it's different fram grade.
	// records used for game play
	PlayerScore score;		// we only store high-score
	PlayerReplayRecord replay;	//
public:
	// used for hashing
	RString Serialize();
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
	PlayerKeyConfig keyconfig;		// key config
public:
	bool LoadPlayerSetting(const RString& name);
	bool SavePlayerSetting(const RString& name);
	/** @brief returns SongRecord object from sqlite db. returns false if not found. */
	/** @brief saves SongRecord object to sqlite db. */
};

namespace PlayerReplayHelper {
	bool LoadReplay(PlayerReplayRecord& rep, const char* playername, const char* songhash, const char* course = 0);
	bool SaveReplay(const PlayerReplayRecord& rep, const char* playername, const char* songhash, const char* course = 0);
}

namespace PlayerRecordHelper {
	bool LoadPlayerRecord(PlayerSongRecord& record, const char* playername, const char* songhash);
	bool SavePlayerRecord(const PlayerSongRecord& record, const char* playername);
	bool DeletePlayerRecord(const char* playername, const char* songhash);
}

namespace PlayerInfoHelper {
	/** @brief set playerinfo as default (COMMENT: don't care playername) */
	void DefaultPlayerInfo(PlayerInfo& player);
	bool LoadPlayerInfo(PlayerInfo& player, const char* playername);
	bool SavePlayerInfo(PlayerInfo& player);
}

namespace PlayerKeyHelper {
	/** @brief get keycode as string. used for KeyConfig */
	RString GetKeyCodeName(int keycode);
	/** @brief get function from keycode. return -1 if none found. (check global.h) */
	int GetKeyCodeFunction(const PlayerKeyConfig &config, int keycode);
	/** @brief load key config from xmlelement. */
	void LoadKeyConfig(PlayerKeyConfig &config, tinyxml2::XMLNode *base);
	/** @brief make key config to a file. use ShallowClone() if you need to include it as element. */
	void SaveKeyConfig(const PlayerKeyConfig &config, tinyxml2::XMLNode *base);

	void DefaultKeyConfig(PlayerKeyConfig &config);
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