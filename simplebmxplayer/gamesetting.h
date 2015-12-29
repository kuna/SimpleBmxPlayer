#pragma once

#define DEFAULT_SETTING_PATH "settings.xml"
#include <string>

class GameSetting {
public:
	struct Engine {
		int mWidth;
		int mHeight;
		bool mFullScreen;
	} mEngine;
	struct Theme {
		std::string main;
		std::string playerselect;
		std::string select;
		std::string play5k;
		std::string play7k;
		std::string play9k;
		std::string play10k;
		std::string play14k;
		std::string result;
	} mTheme;
	int mRecentPlayer;
public:
	bool DefaultSetting();
	bool LoadSetting(const char *path = DEFAULT_SETTING_PATH);
	bool SaveSetting(const char *path = DEFAULT_SETTING_PATH);
};