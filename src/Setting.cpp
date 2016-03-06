#include "Setting.h"
#include "util.h"
#include "tinyxml2.h"
#include "file.h"
#include "Logger.h"

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
		RString file = FILEMANAGER->GetAbsolutePath(SETTINGFILEPATH);

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

		XMLElement *key = settings->FirstChildElement("key");
		if (key) {
			GetStringSafe(key, "keypreset", setting.keypreset_current);
			GetStringSafe(key, "preset4key", setting.keypreset_4key);
			GetStringSafe(key, "preset5key", setting.keypreset_5key);
			GetStringSafe(key, "preset7key", setting.keypreset_7key);
			GetStringSafe(key, "preset8key", setting.keypreset_8key);
			GetStringSafe(key, "preset9key", setting.keypreset_9key);
			GetStringSafe(key, "preset10key", setting.keypreset_10key);
			GetStringSafe(key, "preset14key", setting.keypreset_14key);
			GetStringSafe(key, "preset18key", setting.keypreset_18key);
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
		RString file = FILEMANAGER->GetAbsolutePath(SETTINGFILEPATH);
		RString dir = FILEMANAGER->GetDirectory(file);
		if (!FILEMANAGER->CreateDirectory(dir))
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

		XMLElement *key = doc->NewElement("key");
		settings->LinkEndChild(key);

		AddElement(key, "preset", setting.keypreset_current);
		AddElement(key, "preset4key", setting.keypreset_4key);
		AddElement(key, "preset5key", setting.keypreset_5key);
		AddElement(key, "preset7key", setting.keypreset_7key);
		AddElement(key, "preset8key", setting.keypreset_8key);
		AddElement(key, "preset9key", setting.keypreset_9key);
		AddElement(key, "preset10key", setting.keypreset_10key);
		AddElement(key, "preset14key", setting.keypreset_14key);
		AddElement(key, "preset18key", setting.keypreset_18key);

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

		setting.keypreset_4key = "4key";
		setting.keypreset_5key = "5key";
		setting.keypreset_7key = "7key";
		setting.keypreset_8key = "8key";
		setting.keypreset_9key = "9key";
		setting.keypreset_10key = "10key";
		setting.keypreset_14key = "14key";
		setting.keypreset_18key = "18key";

		setting.username = "NONAME";
		setting.keymode = 7;
		setting.usepreview = 1;
		setting.bmsdirs.clear();

		setting.deltaspeed = 50;
	}
}

void GameSetting::LoadSetting() {
	if (!GameSettingHelper::LoadSetting(*this)) {
		LOG->Warn("Cannot found setting file, Set with default value.");
		GameSettingHelper::DefaultSetting(*this);
	}
	LOG->Info("Loaded Game setting.");
}

void GameSetting::SaveSetting() {
	GameSettingHelper::SaveSetting(*this);
}






namespace {
	int GetIntValue(XMLNode *base, const char *childname) {
		XMLElement *i = base->FirstChildElement(childname);
		if (!i) return 0;
		else {
			if (i->GetText())
				return atoi(i->GetText());
			else return 0;
		}
	}
}

bool KeySetting::LoadKeyConfig(const RString& name) {
	// clear'em first
	//memset(keycode, 0, sizeof(keycode));
	// load
	XMLDocument *doc = new XMLDocument();
	RString abspath = FILEMANAGER->GetAbsolutePath(ssprintf("../setting/keyconfig/%s.xml", name.c_str()));
	if (!doc->LoadFile(abspath)) {
		return false;
	}
	XMLElement *keyconfig = doc->FirstChildElement("KeyConfig");
	for (int i = 0; i < 40; i++) {
		XMLElement *e = keyconfig->FirstChildElement(ssprintf("Key%d", i));
		if (!e) continue;
		for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
			keycode[i][j] = GetIntValue(e, ssprintf("Index%d", j));
		}
	}
	delete doc;
	return true;
}

void KeySetting::SaveKeyConfig(const RString& name) {
	XMLDocument *doc = new XMLDocument();
	XMLElement *keyconfig = doc->NewElement("KeyConfig");
	for (int i = 0; i < 40; i++) {
		XMLElement *e = doc->NewElement(ssprintf("Key%d", i));
		if (!e) continue;
		for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
			XMLElement *e2 = doc->NewElement(ssprintf("Index%d", j));
			e2->SetText(keycode[i][j]);
			e->LinkEndChild(e2);
		}
		keyconfig->LinkEndChild(e);
	}
	RString abspath = FILEMANAGER->GetAbsolutePath(ssprintf("../setting/keyconfig/%s.xml", name.c_str()));
	FILEMANAGER->CreateDirectory(abspath);
	doc->SaveFile(abspath);
	delete doc;
}

void KeySetting::DefaultKeyConfig() {
	keycode[PlayerKeyIndex::P1_BUTTON1][0] = SDL_SCANCODE_Z;
	keycode[PlayerKeyIndex::P1_BUTTON2][0] = SDL_SCANCODE_S;
	keycode[PlayerKeyIndex::P1_BUTTON3][0] = SDL_SCANCODE_X;
	keycode[PlayerKeyIndex::P1_BUTTON4][0] = SDL_SCANCODE_D;
	keycode[PlayerKeyIndex::P1_BUTTON5][0] = SDL_SCANCODE_C;
	keycode[PlayerKeyIndex::P1_BUTTON6][0] = SDL_SCANCODE_F;
	keycode[PlayerKeyIndex::P1_BUTTON7][0] = SDL_SCANCODE_V;
	keycode[PlayerKeyIndex::P1_BUTTON1][1] = 1001;
	keycode[PlayerKeyIndex::P1_BUTTON2][1] = 1002;
	keycode[PlayerKeyIndex::P1_BUTTON3][1] = 1003;
	keycode[PlayerKeyIndex::P1_BUTTON4][1] = 1004;
	keycode[PlayerKeyIndex::P1_BUTTON5][1] = 1005;
	keycode[PlayerKeyIndex::P1_BUTTON6][1] = 1006;
	keycode[PlayerKeyIndex::P1_BUTTON7][1] = 1007;
	keycode[PlayerKeyIndex::P1_BUTTONSCUP][0] = SDL_SCANCODE_LSHIFT;
	keycode[PlayerKeyIndex::P1_BUTTONSCDOWN][0] = SDL_SCANCODE_LCTRL;
	keycode[PlayerKeyIndex::P1_BUTTONSCUP][1] = 1100;					// up
	keycode[PlayerKeyIndex::P1_BUTTONSCDOWN][1] = 1101;					// down
	keycode[PlayerKeyIndex::P1_BUTTONSTART][0] = SDL_SCANCODE_1;

	keycode[PlayerKeyIndex::P2_BUTTON1][0] = SDL_SCANCODE_M;
	keycode[PlayerKeyIndex::P2_BUTTON2][0] = SDL_SCANCODE_K;
	keycode[PlayerKeyIndex::P2_BUTTON3][0] = SDL_SCANCODE_COMMA;
	keycode[PlayerKeyIndex::P2_BUTTON4][0] = SDL_SCANCODE_L;
	keycode[PlayerKeyIndex::P2_BUTTON5][0] = SDL_SCANCODE_PERIOD;
	keycode[PlayerKeyIndex::P2_BUTTON6][0] = SDL_SCANCODE_SEMICOLON;
	keycode[PlayerKeyIndex::P2_BUTTON7][0] = SDL_SCANCODE_SLASH;
	keycode[PlayerKeyIndex::P2_BUTTONSCUP][0] = SDL_SCANCODE_RSHIFT;
	keycode[PlayerKeyIndex::P2_BUTTONSCDOWN][0] = SDL_SCANCODE_RCTRL;
	keycode[PlayerKeyIndex::P2_BUTTONSTART][0] = SDL_SCANCODE_2;
}

namespace PlayerKeyHelper {
	RString GetKeyCodeName(int keycode) {
		// TODO
		return "TODO";
	}

	int GetKeyCodeFunction(const KeySetting &config, int keycode) {
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
				if (config.keycode[i][j] == keycode)
					return i;
			}
		}
		return -1;
	}
}

GameSetting		SETTING;
KeySetting		KEYSETTING;

struct GameSetting_Init {
	GameSetting_Init() {
		/*
		* set default values (from option)
		* (this should be always successful)
		*/
		SETTING.LoadSetting();
		KEYSETTING.LoadKeyConfig(SETTING.keypreset_current);
	}
} _GAMESTETTINGINIT;

