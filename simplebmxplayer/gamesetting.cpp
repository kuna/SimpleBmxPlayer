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
			base->GetDocument()->LinkEndChild(base->GetDocument()->NewElement(name))
				->ToElement()->SetText(value);
		}

		void AddElement(XMLNode *base, const char *name, const char* value) {
			base->GetDocument()->LinkEndChild(base->GetDocument()->NewElement(name))
				->ToElement()->SetText(value);
		}
	}

	bool LoadGameSetting(GameSetting& setting) {
		RString file(SETTINGFILEPATH);
		FileHelper::ConvertPathToAbsolute(file);

		XMLDocument *doc = new XMLDocument();
		if (doc->Parse(file) != 0) {
			delete doc;
			return false;
		}

		setting.width = GetIntSafe(doc, "width", 1280);
		setting.height = GetIntSafe(doc, "height", 760);
		setting.vsync = GetIntSafe(doc, "vsync", 0);
		setting.fullscreen = GetIntSafe(doc, "fullscreen", 1);
		setting.resizable = GetIntSafe(doc, "resizable", 0);
		setting.allowaddon = GetIntSafe(doc, "allowaddon", 1);
		setting.volume = GetIntSafe(doc, "volume", 100);
		setting.tutorial = GetIntSafe(doc, "tutorial", 1);

		XMLElement *skin = doc->FirstChildElement("Skin");
		if (skin) {
			GetStringSafe(skin, "main", setting.skin_main);
			GetStringSafe(skin, "player", setting.skin_player);
			GetStringSafe(skin, "select", setting.skin_select);
			GetStringSafe(skin, "decide", setting.skin_decide);
			GetStringSafe(skin, "5key", setting.skin_play_5key);
			GetStringSafe(skin, "7key", setting.skin_play_7key);
			GetStringSafe(skin, "9key", setting.skin_play_9key);
			GetStringSafe(skin, "10key", setting.skin_play_10key);
			GetStringSafe(skin, "14key", setting.skin_play_14key);
			GetStringSafe(skin, "keyconfig", setting.skin_keyconfig);
			GetStringSafe(skin, "skinconfig", setting.skin_skinconfig);
			GetStringSafe(skin, "common", setting.skin_common);
		}

		GetStringSafe(doc, "username", setting.username);
		setting.keymode = GetIntSafe(doc, "keymode", 7);

		delete doc;
		return true;
	}

	bool SaveGameSetting(GameSetting& setting) {
		RString file(SETTINGFILEPATH);
		FileHelper::ConvertPathToAbsolute(file);
		RString dir = FileHelper::GetParentDirectory(file);
		if (!FileHelper::CreateFolder(dir))
			return false;

		XMLDocument *doc = new XMLDocument();

		AddElement(doc, "width", setting.width);
		AddElement(doc, "height", setting.height);
		AddElement(doc, "vsync", setting.vsync);
		AddElement(doc, "fullscreen", setting.fullscreen);
		AddElement(doc, "resizable", setting.resizable);
		AddElement(doc, "allowaddon", setting.allowaddon);
		AddElement(doc, "volume", setting.volume);
		AddElement(doc, "tutorial", setting.tutorial);

		XMLElement *skin = doc->NewElement("Skin");
		doc->LinkEndChild(skin);

		AddElement(skin, "main", setting.skin_main);
		AddElement(skin, "player", setting.skin_player);
		AddElement(skin, "select", setting.skin_select);
		AddElement(skin, "decide", setting.skin_decide);
		AddElement(skin, "5key", setting.skin_play_5key);
		AddElement(skin, "7key", setting.skin_play_7key);
		AddElement(skin, "9key", setting.skin_play_9key);
		AddElement(skin, "10key", setting.skin_play_10key);
		AddElement(skin, "14key", setting.skin_play_14key);
		AddElement(skin, "keyconfig", setting.skin_keyconfig);
		AddElement(skin, "skinconfig", setting.skin_skinconfig);
		AddElement(skin, "common", setting.skin_common);

		AddElement(doc, "username", setting.username);
		AddElement(doc, "keymode", setting.keymode);

		bool r = doc->SaveFile(file);
		delete doc;
		return r;
	}

	void DefaultGameSetting(GameSetting& setting) {
		setting.width = 1280;
		setting.height = 760;
		setting.vsync = 0;
		setting.fullscreen = 1;
		setting.resizable = 0;
		setting.allowaddon = 1;
		setting.volume = 100;
		setting.tutorial = 1;

		// TODO
		setting.skin_play_7key = "../skin/Wisp_HD/play/HDPLAY_W.lr2skin";
		setting.skin_play_14key = "../skin/Wisp_HD/play/HDPLAY_WDP.lr2skin";

		setting.username = "NONAME";
		setting.keymode = 7;
	}
}
