#include "Setting.h"
#include "util.h"
#include "tinyxml2.h"
#include "file.h"
#include "Logger.h"

#define SETTINGFILEPATH	"../system/settings.xml"
#define INT(s) (atoi(s))
#define SAFE_STR(s) ((s)?(s):"")

using namespace tinyxml2;

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

GameSetting::GameSetting() {
	// set metrics
	IsScoreGraph.SetFromPool("ScoreGraph");
	IsBGA.SetFromPool("Bga");

	// initalize game state
	m_seed = -1;
	m_PlayerCount = 0;
}

void GameSetting::ApplyToMetrics() {
	IsScoreGraph = showgraph;
	IsBGA = bga;
}

bool GameSetting::LoadSetting() {
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

	width = GetIntSafe(settings, "width", 1280);
	height = GetIntSafe(settings, "height", 760);
	vsync = GetIntSafe(settings, "vsync", 0);
	fullscreen = GetIntSafe(settings, "fullscreen", 1);
	resizable = GetIntSafe(settings, "resizable", 0);
	allowaddon = GetIntSafe(settings, "allowaddon", 1);
	volume = GetIntSafe(settings, "volume", 100);
	tutorial = GetIntSafe(settings, "tutorial", 1);
	soundlatency = GetIntSafe(settings, "soundlatency", 1024);
	useIR = GetIntSafe(settings, "useIR", 0);

	// TODO play part
	bga = GetIntSafe(settings, "bga", 1);
	showgraph = true;
	rate = 1.0;
	trainingmode = false;
	startmeasure = 0;
	endmeasure = 1000;
	repeat = 1;
	deltaspeed = GetIntSafe(settings, "deltaspeed", 50);

	XMLElement *skin = settings->FirstChildElement("skin");
	if (skin) {
		GetStringSafe(skin, "main", skin_main);
		GetStringSafe(skin, "player", skin_player);
		GetStringSafe(skin, "select", skin_select);
		GetStringSafe(skin, "decide", skin_decide);
		GetStringSafe(skin, "play5key", skin_play_5key);
		GetStringSafe(skin, "play7key", skin_play_7key);
		GetStringSafe(skin, "play9key", skin_play_9key);
		GetStringSafe(skin, "play10key", skin_play_10key);
		GetStringSafe(skin, "play14key", skin_play_14key);
		GetStringSafe(skin, "keyconfig", skin_keyconfig);
		GetStringSafe(skin, "skinconfig", skin_skinconfig);
		GetStringSafe(skin, "common", skin_common);
	}

	XMLElement *key = settings->FirstChildElement("key");
	if (key) {
		GetStringSafe(key, "keypreset", keypreset_current);
		GetStringSafe(key, "preset4key", keypreset_4key);
		GetStringSafe(key, "preset5key", keypreset_5key);
		GetStringSafe(key, "preset7key", keypreset_7key);
		GetStringSafe(key, "preset8key", keypreset_8key);
		GetStringSafe(key, "preset9key", keypreset_9key);
		GetStringSafe(key, "preset10key", keypreset_10key);
		GetStringSafe(key, "preset14key", keypreset_14key);
		GetStringSafe(key, "preset18key", keypreset_18key);
	}

	GetStringSafe(settings, "username", username);
	keymode = GetIntSafe(settings, "keymode", 7);
	usepreview = GetIntSafe(settings, "usepreview", 1);

	XMLElement *e_bmsdirs = settings->FirstChildElement("bmsdirs");
	for (XMLElement *dir = e_bmsdirs->FirstChildElement("dir"); dir; dir = dir->NextSiblingElement("dir")) {
		bmsdirs.push_back(dir->GetText());
	}

	delete doc;
	return true;
}

bool GameSetting::SaveSetting() {
	RString file = FILEMANAGER->GetAbsolutePath(SETTINGFILEPATH);
	RString dir = FILEMANAGER->GetDirectory(file);
	if (!FILEMANAGER->CreateDirectory(dir))
		return false;

	XMLDocument *doc = new XMLDocument();
	doc->LinkEndChild(doc->NewDeclaration());
	XMLElement *settings = doc->NewElement("setting");
	doc->LinkEndChild(settings);

	AddElement(settings, "width", width);
	AddElement(settings, "height", height);
	AddElement(settings, "vsync", vsync);
	AddElement(settings, "fullscreen", fullscreen);
	AddElement(settings, "resizable", resizable);
	AddElement(settings, "allowaddon", allowaddon);
	AddElement(settings, "volume", volume);
	AddElement(settings, "tutorial", tutorial);
	AddElement(settings, "soundlatency", soundlatency);
	AddElement(settings, "useIR", useIR);

	// TODO play part
	AddElement(settings, "bga", bga);

	XMLElement *skin = doc->NewElement("skin");
	settings->LinkEndChild(skin);

	AddElement(skin, "main", skin_main);
	AddElement(skin, "player", skin_player);
	AddElement(skin, "select", skin_select);
	AddElement(skin, "decide", skin_decide);
	AddElement(skin, "play5key", skin_play_5key);
	AddElement(skin, "play7key", skin_play_7key);
	AddElement(skin, "play9key", skin_play_9key);
	AddElement(skin, "play10key", skin_play_10key);
	AddElement(skin, "play14key", skin_play_14key);
	AddElement(skin, "keyconfig", skin_keyconfig);
	AddElement(skin, "skinconfig", skin_skinconfig);
	AddElement(skin, "common", skin_common);

	XMLElement *key = doc->NewElement("key");
	settings->LinkEndChild(key);

	AddElement(key, "preset", keypreset_current);
	AddElement(key, "preset4key", keypreset_4key);
	AddElement(key, "preset5key", keypreset_5key);
	AddElement(key, "preset7key", keypreset_7key);
	AddElement(key, "preset8key", keypreset_8key);
	AddElement(key, "preset9key", keypreset_9key);
	AddElement(key, "preset10key", keypreset_10key);
	AddElement(key, "preset14key", keypreset_14key);
	AddElement(key, "preset18key", keypreset_18key);

	AddElement(settings, "username", username);
	AddElement(settings, "keymode", keymode);
	AddElement(settings, "usepreview", usepreview);

	XMLElement *e_bmsdirs = doc->NewElement("bmsdirs");
	settings->LinkEndChild(e_bmsdirs);
	for (auto it = bmsdirs.begin(); it != bmsdirs.end(); ++it) {
		AddElement(e_bmsdirs, "dir", *it);
	}

	AddElement(settings, "deltaspeed", deltaspeed);

	bool r = doc->SaveFile(file);
	delete doc;
	return r;
}


void GameSetting::DefaultSetting() {

	width = 1280;
	height = 720;
	vsync = 0;
	fullscreen = 1;
	resizable = 0;
	allowaddon = 1;
	volume = 100;
	tutorial = 1;
	soundlatency = 1024;
	useIR = 0;
	bga = 1;

	// TODO
	skin_play_7key = "../skin/Wisp_HD/play/HDPLAY_W.lr2skin";
	skin_play_14key = "../skin/Wisp_HD/play/HDPLAY_WDP.lr2skin";

	keypreset_4key = "4key";
	keypreset_5key = "5key";
	keypreset_7key = "7key";
	keypreset_8key = "8key";
	keypreset_9key = "9key";
	keypreset_10key = "10key";
	keypreset_14key = "14key";
	keypreset_18key = "18key";

	username = "NONAME";
	keymode = 7;
	usepreview = 1;
	bmsdirs.clear();

	deltaspeed = 50;
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

int KeySetting::GetKeyCodeFunction(int keycode) {
	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < _MAX_KEYCONFIG_MATCH; j++) {
			if (this->keycode[i][j] == keycode)
				return i;
		}
	}
	return -1;
}

namespace PlayerKeyHelper {
	RString GetKeyCodeName(int keycode) {
		// TODO
		return "TODO";
	}
}

GameSetting*	SETTING;
KeySetting*		KEYSETTING;

