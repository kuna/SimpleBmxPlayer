#include "gamesetting.h"
#include "tinyxml2.h"
#include "file.h"

#define SETTINGFILEPATH	"../system/settings.xml"
#define INT(s) (atoi(s))
#define SAFE_STR(s) ((s)?(s):"")

using namespace tinyxml2;

namespace GameSettingHelper {
	namespace {
		int GetIntSafe(XMLNode *base, const char* name, int default = 0) {
			if (!base) return default;
			XMLElement *e = base->FirstChildElement(name);
			if (!e) return default;
			if (!e->GetText()) return default;
			return atoi(e->GetText());
		}

		bool GetStringSafe(XMLNode *base, const char* name, RString& out) {
			if (!base) return false;
			XMLElement *e = base->FirstChildElement(name);
			if (!e) return false;
			if (!e->GetText()) return false;
			out = e->GetText();
			return true;
		}

		void AddElement(XMLNode *base, const char *name, int value) {
			base->LinkEndChild(base->GetDocument()->NewElement(name))
				->ToElement()->SetText(value);
		}

		void AddElement(XMLNode *base, const char *name, const char* value) {
			base->LinkEndChild(base->GetDocument()->NewElement(name))
				->ToElement()->SetText(value);
		}
	}

	bool LoadSetting(GameSetting& setting) {
		RString file(SETTINGFILEPATH);
		FileHelper::ConvertPathToAbsolute(file);

		XMLDocument *doc = new XMLDocument();
		if (doc->LoadFile(file) != 0) {
			delete doc;
			return false;
		}
		XMLElement *settings = doc->FirstChildElement("setting");
		if (!settings) {
			delete doc;
			return false;
		}

		setting.width = GetIntSafe(settings, "width", 1280);
		setting.height = GetIntSafe(settings, "height", 760);
		setting.vsync = GetIntSafe(settings, "vsync", 0);
		setting.fullscreen = GetIntSafe(settings, "fullscreen", 1);
		setting.resizable = GetIntSafe(settings, "resizable", 0);
		setting.allowaddon = GetIntSafe(settings, "allowaddon", 1);
		setting.volume = GetIntSafe(settings, "volume", 100);
		setting.tutorial = GetIntSafe(settings, "tutorial", 1);
		setting.soundlatency = GetIntSafe(settings, "soundlatency", 1024);
		setting.useIR = GetIntSafe(settings, "useIR", 0);
		setting.bga = GetIntSafe(settings, "bga", 1);

		XMLElement *skin = settings->FirstChildElement("skin");
		if (skin) {
			GetStringSafe(skin, "main", setting.skin_main);
			GetStringSafe(skin, "player", setting.skin_player);
			GetStringSafe(skin, "select", setting.skin_select);
			GetStringSafe(skin, "decide", setting.skin_decide);
			GetStringSafe(skin, "play5key", setting.skin_play_5key);
			GetStringSafe(skin, "play7key", setting.skin_play_7key);
			GetStringSafe(skin, "play9key", setting.skin_play_9key);
			GetStringSafe(skin, "play10key", setting.skin_play_10key);
			GetStringSafe(skin, "play14key", setting.skin_play_14key);
			GetStringSafe(skin, "keyconfig", setting.skin_keyconfig);
			GetStringSafe(skin, "skinconfig", setting.skin_skinconfig);
			GetStringSafe(skin, "common", setting.skin_common);
		}

		GetStringSafe(settings, "username", setting.username);
		setting.keymode = GetIntSafe(settings, "keymode", 7);
		setting.usepreview = GetIntSafe(settings, "usepreview", 1);

		XMLElement *bmsdirs = settings->FirstChildElement("bmsdirs");
		for (XMLElement *dir = bmsdirs->FirstChildElement("dir"); dir; dir = dir->NextSiblingElement("dir")) {
			setting.bmsdirs.push_back(dir->GetText());
		}

		setting.deltaspeed = GetIntSafe(settings, "deltaspeed", 50);

		delete doc;
		return true;
	}

	bool SaveSetting(const GameSetting& setting) {
		RString file(SETTINGFILEPATH);
		FileHelper::ConvertPathToAbsolute(file);
		RString dir = FileHelper::GetParentDirectory(file);
		if (!FileHelper::CreateFolder(dir))
			return false;

		XMLDocument *doc = new XMLDocument();
		doc->LinkEndChild(doc->NewDeclaration());
		XMLElement *settings = doc->NewElement("setting");
		doc->LinkEndChild(settings);

		AddElement(settings, "width", setting.width);
		AddElement(settings, "height", setting.height);
		AddElement(settings, "vsync", setting.vsync);
		AddElement(settings, "fullscreen", setting.fullscreen);
		AddElement(settings, "resizable", setting.resizable);
		AddElement(settings, "allowaddon", setting.allowaddon);
		AddElement(settings, "volume", setting.volume);
		AddElement(settings, "tutorial", setting.tutorial);
		AddElement(settings, "soundlatency", setting.soundlatency);
		AddElement(settings, "useIR", setting.useIR);
		AddElement(settings, "bga", setting.bga);

		XMLElement *skin = doc->NewElement("skin");
		settings->LinkEndChild(skin);

		AddElement(skin, "main", setting.skin_main);
		AddElement(skin, "player", setting.skin_player);
		AddElement(skin, "select", setting.skin_select);
		AddElement(skin, "decide", setting.skin_decide);
		AddElement(skin, "play5key", setting.skin_play_5key);
		AddElement(skin, "play7key", setting.skin_play_7key);
		AddElement(skin, "play9key", setting.skin_play_9key);
		AddElement(skin, "play10key", setting.skin_play_10key);
		AddElement(skin, "play14key", setting.skin_play_14key);
		AddElement(skin, "keyconfig", setting.skin_keyconfig);
		AddElement(skin, "skinconfig", setting.skin_skinconfig);
		AddElement(skin, "common", setting.skin_common);

		AddElement(settings, "username", setting.username);
		AddElement(settings, "keymode", setting.keymode);
		AddElement(settings, "usepreview", setting.usepreview);

		XMLElement *bmsdirs = doc->NewElement("bmsdirs");
		settings->LinkEndChild(bmsdirs);
		for (auto it = setting.bmsdirs.begin(); it != setting.bmsdirs.end(); ++it) {
			AddElement(bmsdirs, "dir", *it);
		}

		AddElement(settings, "deltaspeed", setting.deltaspeed);

		bool r = doc->SaveFile(file);
		delete doc;
		return r;
	}

	void DefaultSetting(GameSetting& setting) {
		setting.width = 1280;
		setting.height = 720;
		setting.vsync = 0;
		setting.fullscreen = 1;
		setting.resizable = 0;
		setting.allowaddon = 1;
		setting.volume = 100;
		setting.tutorial = 1;
		setting.soundlatency = 1024;
		setting.useIR = 0;
		setting.bga = 1;

		// TODO
		setting.skin_play_7key = "../skin/Wisp_HD/play/HDPLAY_W.lr2skin";
		setting.skin_play_14key = "../skin/Wisp_HD/play/HDPLAY_WDP.lr2skin";

		setting.username = "NONAME";
		setting.keymode = 7;
		setting.usepreview = 1;
		setting.bmsdirs.clear();

		setting.deltaspeed = 50;
	}
}
