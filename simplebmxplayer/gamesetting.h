#pragma once

#include "global.h"

struct GameSetting {
	// basic information
	int width, height;
	int vsync, fullscreen, resizable;
	int allowaddon;
	int volume;
	int soundlatency;
	int tutorial;

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

	// last select user on selectuser screen
	RString username;

	// song select
	int keymode;

	// game play
	int speed

	// result screen
	// - NOPE
};

namespace GameSettingHelper {
	bool LoadSetting(GameSetting&);
	bool SaveSetting(const GameSetting&);
	void DefaultSetting(GameSetting&);
}