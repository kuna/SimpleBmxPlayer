#include "gamesetting.h"
#include "tinyxml2.h"
#include "file.h"

#define INT(s) (atoi(s))
#define SAFE_STR(s) ((s)?(s):"")

using namespace tinyxml2;

bool GameSetting::DefaultSetting() {
	return LoadSetting();
}

bool GameSetting::LoadSetting(const char *path) {
	RString abspath = path;
	FileHelper::ConvertPathToAbsolute(abspath);
	XMLDocument doc;
	if (doc.LoadFile(abspath) != XML_NO_ERROR)
		return false;

	XMLElement* ele_setting = doc.FirstChildElement("settings");
	if (ele_setting) {
		try {
			XMLElement* ele_engine = ele_setting->FirstChildElement("engine");
			XMLElement* ele_theme = ele_setting->FirstChildElement("theme");

			mEngine.mWidth = INT(ele_engine->FirstChildElement("width")->GetText());
			mEngine.mHeight = INT(ele_engine->FirstChildElement("height")->GetText());
			mEngine.mFullScreen = INT(ele_engine->FirstChildElement("fullscreen")->GetText()) == 1;
			
			mTheme.main = SAFE_STR(ele_theme->FirstChildElement("main")->GetText());
			mTheme.playerselect = SAFE_STR(ele_theme->FirstChildElement("playerselect")->GetText());
			mTheme.select = SAFE_STR(ele_theme->FirstChildElement("select")->GetText());
			mTheme.play5k = SAFE_STR(ele_theme->FirstChildElement("play5k")->GetText());
			mTheme.play7k = SAFE_STR(ele_theme->FirstChildElement("play7k")->GetText());
			mTheme.play9k = SAFE_STR(ele_theme->FirstChildElement("play9k")->GetText());
			mTheme.play10k = SAFE_STR(ele_theme->FirstChildElement("play10k")->GetText());
			mTheme.play14k = SAFE_STR(ele_theme->FirstChildElement("play14k")->GetText());

			mRecentPlayer = INT(ele_setting->FirstChildElement("player")->FirstChildElement("recent")->GetText());
		}
		catch (...) {
			return false;
		}
	}
	else {
		return false;
	}

	return true;
}

bool GameSetting::SaveSetting(const char *path) {
	XMLDocument doc;

	XMLElement *ele_setting = doc.NewElement("settings");
	if (ele_setting) {
		XMLElement *ele_engine = doc.NewElement("engine");
		XMLElement *ele_theme = doc.NewElement("theme");
		XMLElement *ele_player = doc.NewElement("player");

		if (ele_engine) {
			XMLElement *ele_width = doc.NewElement("width");
			XMLElement *ele_height = doc.NewElement("height");
			XMLElement *ele_fullscreen = doc.NewElement("fullscreen");

			XMLText *width = doc.NewText(std::to_string(mEngine.mWidth).c_str());
			XMLText *height = doc.NewText(std::to_string(mEngine.mHeight).c_str());
			XMLText *fullscreen = doc.NewText(std::to_string(mEngine.mFullScreen ? 1 : 0).c_str());

			ele_width->LinkEndChild(width);
			ele_height->LinkEndChild(height);
			ele_fullscreen->LinkEndChild(fullscreen);
			ele_engine->LinkEndChild(ele_width);
			ele_engine->LinkEndChild(ele_height);
			ele_engine->LinkEndChild(ele_fullscreen);
		}

		if (ele_theme) {
			XMLElement *main = doc.NewElement("main");
			XMLElement *playerselect = doc.NewElement("playerselect");
			XMLElement *select = doc.NewElement("select");
			XMLElement *play5k = doc.NewElement("play5k");
			XMLElement *play7k = doc.NewElement("play7k");
			XMLElement *play9k = doc.NewElement("play9k");
			XMLElement *play10k = doc.NewElement("play10k");
			XMLElement *play14k = doc.NewElement("play14k");
			XMLElement *result = doc.NewElement("result");

			main->LinkEndChild(doc.NewText(mTheme.main.c_str()));
			playerselect->LinkEndChild(doc.NewText(mTheme.playerselect.c_str()));
			select->LinkEndChild(doc.NewText(mTheme.select.c_str()));
			play5k->LinkEndChild(doc.NewText(mTheme.play5k.c_str()));
			play7k->LinkEndChild(doc.NewText(mTheme.play7k.c_str()));
			play9k->LinkEndChild(doc.NewText(mTheme.play9k.c_str()));
			play10k->LinkEndChild(doc.NewText(mTheme.play10k.c_str()));
			play14k->LinkEndChild(doc.NewText(mTheme.play14k.c_str()));
			result->LinkEndChild(doc.NewText(mTheme.result.c_str()));

			ele_theme->LinkEndChild(main);
			ele_theme->LinkEndChild(playerselect);
			ele_theme->LinkEndChild(select);
			ele_theme->LinkEndChild(play5k);
			ele_theme->LinkEndChild(play7k);
			ele_theme->LinkEndChild(play9k);
			ele_theme->LinkEndChild(play10k);
			ele_theme->LinkEndChild(play14k);
			ele_theme->LinkEndChild(result);
		}

		if (ele_player) {
			XMLElement *recent = doc.NewElement("recent");
			recent->LinkEndChild(doc.NewText(std::to_string(mRecentPlayer).c_str()));
			ele_player->LinkEndChild(recent);
		}

		ele_setting->LinkEndChild(ele_engine);
		ele_setting->LinkEndChild(ele_theme);
		ele_setting->LinkEndChild(ele_player);
	}
	doc.LinkEndChild(ele_setting);
	return (doc.SaveFile(path) == XML_NO_ERROR);
}