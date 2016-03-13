#pragma once

#include "global.h"
#include "Theme.h"
#include <vector>

class GameSetting {
public:
	GameSetting();

	// basic setting
	int width, height;
	int vsync, fullscreen, resizable;
	int allowaddon;
	int volume;
	int soundlatency;
	int tutorial;
	int useIR;

	// play setting (common)
	int bga;
	int showgraph;
	double rate;
	bool trainingmode;
	int startmeasure, endmeasure, repeat;
	int deltaspeed;

	// skin (not skin option)
	RString skin_main;
	RString skin_player;
	RString skin_select;
	RString skin_decide;
	RString skin_play_5key;
	RString skin_play_7key;
	RString skin_play_9key;
	RString skin_play_10key;
	RString skin_play_14key;
	RString skin_result;
	RString skin_keyconfig;
	RString skin_skinconfig;
	RString skin_common;

	// keysetting preset
	RString keypreset_current;
	RString keypreset_4key;		// Stepmania SP
	RString keypreset_5key;		// BMS 5Key assisted	( depreciated? )
	RString keypreset_7key;		// BMS/O2Jam SP
	RString keypreset_8key;		// Stepmania DP (or EZ2DJ 7key)
	RString keypreset_9key;		// PMS
	RString keypreset_10key;	// BMS 10key assisted ( depreciated? )
	RString keypreset_14key;	// 
	RString keypreset_18key;	// PMS DP (or EZ2DJ 18key)

	// last select user on selectuser screen
	RString username;

	// play setting
	int keymode;				// keymode
	int gamemode;				// gamemode (course, normal ...)
	int battlemode;				// 
	int playside;
	int autoplay;
	int networkplay;
	int usepreview;
	std::vector<RString> bmsdirs;

	// result screen
	// - NOPE

	void DefaultSetting();
	bool LoadSetting();
	bool SaveSetting();
	void ApplyToMetrics();
	bool IsAssisted();			// global option is assisted?
public:
	// Game state (not saved)
	// playing related
	int m_seed;
	int m_PlayerCount;			// ?? currently playing player count
	RString m_CoursePath[10];
	RString m_CourseHash[10];
	int m_CourseRound;
	int m_CourseCount;
	RString m_2PSongPath[10];	// not generally used
	RString m_2PSongHash[10];	// not generally used
private:
	// metrics
	// TODO
	// BGA, Keymode, ..
	SwitchValue		IsScoreGraph;
	SwitchValue		IsBGA;
};

extern GameSetting*		SETTING;





/*
 * this may be located at Input class...
 * but I think Setting is more suitable, as it stored in settings folder.
 */
#define _MAX_KEYCONFIG_MATCH	8

class KeySetting {
public:
	int keycode[40][_MAX_KEYCONFIG_MATCH];

	bool LoadKeyConfig(const RString& name);
	void SaveKeyConfig(const RString& name);
	void DefaultKeyConfig();
	void ApplyThemeMetrics();
	/** @brief get function from keycode. return -1 if none found. (check global.h) */
	int GetKeyCodeFunction(int keycode);
public:
	// theme metrics
	// TODO
};

namespace PlayerKeyHelper {
	/** @brief get keycode as string. used for KeyConfig */
	RString GetKeyCodeName(int keycode);
}

extern KeySetting*		KEYSETTING;