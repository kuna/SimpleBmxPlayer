#pragma once

#include "global.h"
#include <vector>

struct GameSetting {
	// basic information
	int width, height;
	int vsync, fullscreen, resizable;
	int allowaddon;
	int volume;
	int soundlatency;
	int tutorial;
	int useIR;
	int bga;

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

	// song select
	int keymode;
	int usepreview;
	std::vector<RString> bmsdirs;

	// game play
	int deltaspeed;

	// result screen
	// - NOPE

	void LoadSetting();
	void SaveSetting();
};

namespace GameSettingHelper {
	bool LoadSetting(GameSetting&);
	bool SaveSetting(const GameSetting&);
	void DefaultSetting(GameSetting&);
}

extern GameSetting		SETTING;





/*
 * this may be located at Input class...
 * but I think Setting is more suitable, as it stored in settings folder.
 */
#define _MAX_KEYCONFIG_MATCH	8

struct KeySetting {
	int keycode[40][_MAX_KEYCONFIG_MATCH];

	bool LoadKeyConfig(const RString& name);
	void SaveKeyConfig(const RString& name);
	void DefaultKeyConfig();
};

namespace PlayerKeyHelper {
	/** @brief get keycode as string. used for KeyConfig */
	RString GetKeyCodeName(int keycode);
	/** @brief get function from keycode. return -1 if none found. (check global.h) */
	int GetKeyCodeFunction(const KeySetting &config, int keycode);
}

extern KeySetting		KEYSETTING;