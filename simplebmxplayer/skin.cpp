#include "skin.h"
#include "skinelement.h"
#include "game.h"

// temporarily used common function
char* Trim(char *p) {
	char *r = p;
	while (*p == ' ' || *p == '\t')
		p++;
	r = p;
	while (*p != 0)
		p++;
	p--;
	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != 0)
		p--;
	*p = 0;
	return r;
}

// ---------------------------------------------------------------

bool _LR2SkinParser::ParseLR2Skin(const char *filepath, Skin *s, SkinOption *option) {
	this->s = s;
	this->option = option;

	// fill basic information to skin
	strcpy(s->filepath, filepath);
	const char *fn = strrchr(filepath, '/');
	if (!fn) fn = strrchr(filepath, '\\');
	if (!fn) fn = filepath;
	strcpy(s->skinname, fn + 1);
	//IO::make_filename_safe(s->skinname);	TODO

	// skin parse start
	Parse(filepath);

	// after parsing end,
	// check out is there any left elements that needs to be generated
	for (int i = 0; i < 100; i++) {
		if (element[i].IsValid()) {
			SkinElement* conv_element = ConvertToElement(&element[i]);
			if (conv_element) {
				s->AddElement(conv_element);
				delete conv_element;
			}
		}
	}

	if (parser_level == 0) {
		printf("lr2skin(%s) parsing successfully done", filepath);
		return true;
	}
	else {
		// if this value is the final output,
		// then there might be some #ENDIF was leaked.
		return false;
	}
}

void _LR2SkinParser::Parse(const char *filepath) {
	FILE *f = fopen(filepath, "r");
	if (!f) {
		printf("Cannot find Skin file %s - ignore", filepath);
		return;
	}

	char line[10240];
	char *p;
	char *args[50];
	current_line = 1;
	while (!feof(f)) {
		fgets(line, 1024, f);
		p = Trim(line);
		args[0] = p;
		int i;
		for (i = 1; i < 50; i++) {
			p = strchr(p, ',');
			if (!p)
				break;
			*p = 0;
			args[i] = (++p);
		}
		for (; i < 50; i++) args[i] = 0;	/* for safety! */
		ParseLR2SkinArgs(args);
		current_line++;
	}

	fclose(f);
}

void _LR2SkinParser::ParseLR2SkinArgs(char **args) {
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
#define OBJTYPE_IS(v) (strcmp(args[0]+4, (v)) == 0)
#define CMD_STARTSWITH(v,l) (strncmp(args[0], (v), (l)) == 0)
#define INT(v) (atoi(v))
#define ADDTOHEADER(v) (v)
	/*
	 * ignore comment
	 */
	if (CMD_STARTSWITH("//", 2))
		return;

	/*
	 * condition / stack part
	 */
	if (CMD_IS("#ENDIF")) {
		for (int i = 0; i < parser_level_condition_num[parser_level]; i++)
			parser_condition.pop_back();
		parser_level--;
	}
	else if (CMD_IS("#ELSEIF")) {
		// #ELSEIF clause may NOT be exact - avoid them if you can!
		// erase previous condition
		for (int i = 0; i < parser_level_condition_num[parser_level]; i++)
			parser_condition.pop_back();
		parser_level_condition_num[parser_level] = 0;
		for (int i = 1; i < 50 && atoi(args[i]); i++) {
			parser_condition.push_back(atoi(args[i]));
			parser_level_condition_num[parser_level]++;
		}
	}
	else if (CMD_IS("#ELSE")) {
		// #ELSE clause may NOT be exact - avoid them if you can!
		// invert all previous condition
		int i = parser_condition.size();
		for (int _i = 0; i < parser_level_condition_num[parser_level]; i++) {
			parser_condition[--i] *= -1;
		}
	}
	else if (CMD_IS("#IF")) {
		parser_level_condition_num[++parser_level] = 0;
		for (int i = 1; i < 50 && atoi(args[i]); i++) {
			parser_condition.push_back(atoi(args[i]));
			parser_level_condition_num[parser_level]++;
		}
	}

	/*
	 * header/metadata parsing
	 */
	if (CMD_IS("#CUSTOMOPTION")) {
		std::string key = args[1];
		int optionid = atoi(args[2]);
		if (!option->IsOptionKeyExists(args[1])) {
			option->ModifyOption(args[1], 0);
			option_id_name.insert(std::pair<int, std::string>(optionid, key));
		}
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		// default value is ignored.
		std::string key = args[1];
		std::string filterpath = args[2];
		if (!option->IsFileOptionKeyExists(key)) {
			int idx = filterpath.find_last_of('*');
			std::string basicpath;
			if (idx == std::string::npos) {
				basicpath = filterpath;
			}
			else {
				basicpath = filterpath.substr(0, idx) + args[3] + filterpath.substr(idx + 1);
			}
			option->ModifyFileOption(args[1], basicpath);		// set to basic value
			option_fn_name.insert(std::pair<std::string, std::string>(filterpath, key));
		}
	}
	else if (CMD_IS("#INFORMATION")) {
		// set skin's metadata
		int type = INT(args[2]);		// 0: 7key, 1: 9key, 2: 14key, 12: battle
		if (type == 0) type = 7;
		else if (type == 1) type = 9;
		else if (type == 2) type = 14;
		else if (type == 12) type = 77;
		s->type = type;
		strcpy(s->skinname, args[3]);
		strcpy(s->author, args[4]);
	}
	else if (CMD_IS("#INCLUDE")) {
		parser_level++;
		Parse(args[1]);
		parser_level--;
	}
	else if (CMD_IS("#IMAGE")) {
		// It's easy - we just push back file path.
		// but Caution: If key value exists in option, then we should get filepath from there
		// TODO: but we have to change ./LR2files~ path automatically, maybe ...
		if (option_fn_name.find(args[1]) != option_fn_name.end())
			s->resource_imgs.push_back(option->GetFileOption(option_fn_name[args[1]]));
		else
			s->resource_imgs.push_back(args[1]);
	}
	else if (CMD_IS("#FONT")) {
		// we don't use bitmap fonts
		// So if cannot found, we'll going to use default font/texture.
		// current font won't support TTF, so basically we're going to use default font.
		s->resource_fonts.push_back({
			args[4],			// filepath
			"",					// texturepath
			INT(args[1]),		// size
			INT(args[3]),		// type (0: normal, 1: italic, 2: bold)
			INT(args[2]),		// thickness
			1					// font border
		});
	}
	else if (CMD_IS("#SETOPTION")) {
		// check current if statement condition...
		bool cond = true;
		for (int i = 0; i < parser_condition.size() && cond; i++) {
			if (!current_skin_option[parser_condition[i]])
				cond = false;
		}
		if (cond) {
			parser_level_condition_num[parser_level]++;
			parser_condition.push_back(INT(args[1]));
		}
	}
	/* Just ignore these option */
	else if (CMD_IS("#ENDOFHEADER")) {}
	else if (CMD_IS("#FLIPRESULT")) {}
	else if (CMD_IS("#TRANSCLOLR")) {}		// should we have to implement colorkey?
	else if (CMD_STARTSWITH("#SRC_", 4) || CMD_STARTSWITH("#DST_", 4)){
		std::string objectname = args[0] + 4;
		int objectid = INT(args[1]);
		// there's some objects need to be cautioned ...
		// (aware channel duplication)
		if (OBJTYPE_IS("LN_END"))
			objectid += 10;
		if (OBJTYPE_IS("LN_BODY"))
			objectid += 20;
		if (OBJTYPE_IS("LN_START"))
			objectid += 30;		// 50
		if (OBJTYPE_IS("LN_MINE"))
			objectid += 60;
		if (OBJTYPE_IS("AUTO_NOTE"))
			objectid = 70;
		if (OBJTYPE_IS("AUTO_LN_END"))
			objectid = 73;
		if (OBJTYPE_IS("AUTO_LN_BODY"))
			objectid = 72;
		if (OBJTYPE_IS("AUTO_LN_START"))
			objectid = 71;
		if (OBJTYPE_IS("AUTO_MINE"))
			objectid = 74;

		// start to fill object in current channel
		_LR2SkinElement *e = &element[objectid];
		SkinElement* conv_element = 0;
		if (CMD_STARTSWITH("#SRC", 3)) {
			// if current channel is dirty
			// then flush current channel
			if (e->IsValid()) {
				conv_element = ConvertToElement(e);
				e->Clear();
				if (conv_element) {
					s->AddElement(conv_element);
					delete conv_element;
				}
			}

			e->CopyOptions(parser_condition);
			e->objname = args[0] + 4;
			e->AddSrc(args + 1);
		}
		else if (CMD_STARTSWITH("#DST", 3)) {
			e->AddDst(args + 1);
		}
	}
	else {
		printf("Unknown Type: %s (%dL) - Ignore.", args[0], current_line);
	}
#undef CMD_IS
}

SkinElement* _LR2SkinParser::ConvertToElement(_LR2SkinElement *e) {
	COMMON::upper(e->objname);
	// first check whether is it hiding or showing command
	bool ishiding = false;
	if (e->GetLastDst()->a == 0)
		ishiding = true;

	SkinDebugInfo debugInfo;
	debugInfo.line = current_line;

	SkinElement* newobj = 0;

	// find some specific tags
	if (e->objname == "IMAGE") {
		if (e->resource_id == 110) {
			// I don't know what it is, it's called bga mask ...
			// but is it necessary ..?
			printf("[INFO] ignore BGA MASK option (%dL)", current_line);
			return 0;
		}

		// create new object
		newobj = new SkinElement(debugInfo);
	}
	else if (e->objname == "BGA") {
		// we shouldn't set ID here because many BGA can be existed.
		newobj = new SkinElement(debugInfo);
		newobj->AddClassName("BGA");
	}
	else if (e->objname == "BARGRAPH") {
		// create as graph element
		newobj = new SkinGraphElement(debugInfo);

		// set graph type
		((SkinGraphElement*)newobj)->SetDirection(e->muki);
		// set id
		if (e->value_id == 12)
			newobj->SetID("1PCurrentHighScoreGraph");
		if (e->value_id == 13)
			newobj->SetID("1PHighScoreGraph");
		if (e->value_id == 13)
			newobj->SetID("1PCurrentExScoreGraph");
		if (e->value_id == 15)
			newobj->SetID("1PExScoreGraph");
	}
	else if (e->objname == "TEXT") {
		// create as text element
		newobj = new SkinTextElement(debugInfo);

		// set text type
		// TODO
		((SkinTextElement*)newobj)->SetEditable(0);
		((SkinTextElement*)newobj)->SetAlign(0);

		// set id
		if (e->value_id == 10) {
			newobj->SetID("SongTitle");
		}
		else if (e->value_id == 11) {
			newobj->SetID("SongSubTitle");
		}
		else if (e->value_id == 12) {
			newobj->SetID("SongDisplayTitle");
		}
		else if (e->value_id == 13) {
			newobj->SetID("SongGenre");
		}
		else if (e->value_id == 14) {
			newobj->SetID("SongArtist");
		}
		else if (e->value_id == 15) {
			newobj->SetID("SongSubArtist");
		}
		else if (e->value_id == 16) {
			newobj->SetID("SongSearchTag");
		}
	}
	else if (e->objname == "NUMBER")
	{
		// sprite number follows format, and only supports int. (likely depreciated)
		// if you want more varisity then use Text.
		if (e->divx <= 1 && e->divy <= 1) {
			printf("[ERROR] Errorneous #XXX_NUMBER object (%dL). ignore.\n", current_line);
			return 0;
		}

		// src will be automatically processed after this
		// so we don't care here.

		// now set ID.
		if (e->value_id == 101)
			newobj->SetID("1PExScore");
		if (e->value_id == 105)
			newobj->SetID("1PMaxCombo");
		if (e->value_id == 151)
			newobj->SetID("1PCurrentHighScore");
		if (e->value_id == 151)
			newobj->SetID("1PCurrentTargetScore");
		if (e->value_id == 161)
			newobj->SetID("RemainingMinute");
		if (e->value_id == 162)
			newobj->SetID("RemainingSecond");
	}
	else if (e->objname == "JUDGELINE") {
		// It has not much meaning ...
		newobj = new SkinElement(debugInfo);
		newobj->SetID("1PJudgeLine");
	}
	else if (e->objname == "LINE") {
		// TODO
		newobj = new SkinElement(debugInfo);
		newobj->SetID("1PBeatLine");
	}
	else if (e->objname == "NOTE") {
		// TODO
		newobj = new SkinElement(debugInfo);
	}
	else if (e->objname == "LN_END") {
		// TODO
		newobj = new SkinElement(debugInfo);
	}
	else if (e->objname == "LN_BODY") {
		// TODO
		newobj = new SkinElement(debugInfo);
	}
	else if (e->objname == "BUTTON") {
		// TODO
		newobj = new SkinButtonElement(debugInfo);
	}
	else if (e->objname == "SLIDER") {
		// TODO
		newobj = new SkinSliderElement(debugInfo);

		if (e->value_id == 1) {
			// this is global slider, used in all select menu
			newobj->SetID("SelectSlider");
		}
		else if (e->value_id == 2) {
			newobj->SetID("1PHighSpeed");	// what does it means?
		}
		else if (e->value_id == 3) {
			newobj->SetID("2PHighSpeed");	// what does it means?
		}
		else if (e->value_id == 4) {
			/*
			 * Sudden/Lift, all of them are setted by judgeline's xywh,
			 * So we ignore all of the dst option of this slider.
			 */
			newobj->SetID("1PSudden");
		}
		else if (e->value_id == 5) {
			newobj->SetID("2PSudden");
		}
		else if (e->value_id == 6) {
			newobj->SetID("SongProgress");
		}
	}
	// we won't support ONMOUSE object.

	/*
	 * we're figured out it's a valid object
	 * and decided to make object solid
	 */
	if (newobj) {
		// make classname from conditions
		if (e->CheckOption(1)) {
			newobj->AddClassName("OnClose");
		}
		if (e->CheckOption(32)) {
			newobj->AddClassName("AutoPlay");
		}
		if (e->CheckOption(33)) {
			newobj->AddClassName("AutoPlayOff");
		}
		if (e->CheckOption(34)) {
			newobj->AddClassName("GhostOff");
		}
		if (e->CheckOption(35)) {
			newobj->AddClassName("GhostA");
		}
		if (e->CheckOption(36)) {
			newobj->AddClassName("GhostB");
		}
		if (e->CheckOption(37)) {
			newobj->AddClassName("GhostC");
		}
		if (e->CheckOption(38)) {
			newobj->AddClassName("ScoreGraphOff");
		}
		if (e->CheckOption(39)) {
			newobj->AddClassName("ScoreGraph");
		}
		if (e->CheckOption(40)) {
			newobj->AddClassName("BGAOff");
		}
		if (e->CheckOption(41)) {
			newobj->AddClassName("BGAOn");
		}
		if (e->CheckOption(42)) {
			newobj->AddClassName("1PGrooveGuage");
		}
		if (e->CheckOption(43)) {
			newobj->AddClassName("1PHardGuage");
		}
		if (e->CheckOption(44)) {
			newobj->AddClassName("2PGrooveGuage");
		}
		if (e->CheckOption(45)) {
			newobj->AddClassName("2PHardGuage");
		}
		if (e->CheckOption(46)) {
			newobj->AddClassName("2PHardGuage");
		}
		if (e->CheckOption(47)) {
			newobj->AddClassName("2PHardGuage");
		}
		if (e->CheckOption(50)) {
			newobj->AddClassName("Offline");
		}
		if (e->CheckOption(51)) {
			newobj->AddClassName("Online");
		}
		if (e->CheckOption(80)) {
			newobj->AddClassName("OnSongLoadingStart");
		}
		if (e->CheckOption(81)) {
			newobj->AddClassName("OnSongLoadingEnd");
		}
		if (e->CheckOption(151)) {
			newobj->AddClassName("Beginner");
		}
		if (e->CheckOption(152)) {
			newobj->AddClassName("Normal");
		}
		if (e->CheckOption(153)) {
			newobj->AddClassName("Hyper");
		}
		if (e->CheckOption(154)) {
			newobj->AddClassName("Another");
		}
		if (e->CheckOption(155)) {
			newobj->AddClassName("Insane");
		}
		if (e->CheckOption(270)) {
			newobj->AddClassName("On1PSuddenChange");
		}
		if (e->CheckOption(271)) {
			newobj->AddClassName("On2PSuddenChange");
		}
		// TODO: Skin option effect - how can we process it?

		// make classname from timer
		if (e->timer == 2) {
			newobj->AddClassName("OnClose");	// FADEOUT
		}
		else if (e->timer == 3) {
			newobj->AddClassName("OnFail");	// Stage failed
		}
		else if (e->timer == 40) {
			newobj->AddClassName("OnReady");
		}
		else if (e->timer == 41) {
			newobj->AddClassName("OnGameStart");
		}
		else if (e->timer == 42) {
			newobj->AddClassName("On1PGuageUp");
		}
		else if (e->timer == 43) {
			newobj->AddClassName("On2PGuageUp");
		}
		else if (e->timer == 44) {
			newobj->AddClassName("On1PGuageMax");
		}
		else if (e->timer == 45) {
			newobj->AddClassName("On2PGuageMax");
		}
		else if (e->timer == 46) {
			newobj->AddClassName("On1PJudge");
		}
		else if (e->timer == 47) {
			newobj->AddClassName("On2PJudge");
		}
		else if (e->timer == 48) {
			newobj->AddClassName("On1PFullCombo");
		}
		else if (e->timer == 49) {
			newobj->AddClassName("On2PFullCombo");
		}
		else if (e->timer == 50) {
			newobj->AddClassName("On1PKeySCGreat");
		}
		else if (e->timer == 51) {
			newobj->AddClassName("On1PKey1Great");
		}
		else if (e->timer == 52) {
			newobj->AddClassName("On1PKey2Great");
		}
		else if (e->timer == 53) {
			newobj->AddClassName("On1PKey3Great");
		}
		else if (e->timer == 54) {
			newobj->AddClassName("On1PKey4Great");
		}
		else if (e->timer == 55) {
			newobj->AddClassName("On1PKey5Great");
		}
		else if (e->timer == 56) {
			newobj->AddClassName("On1PKey6Great");
		}
		else if (e->timer == 57) {
			newobj->AddClassName("On1PKey7Great");
		}
		else if (e->timer == 58) {
			newobj->AddClassName("On1PKey8Great");
		}
		else if (e->timer == 59) {
			newobj->AddClassName("On1PKey9Great");
		}
		else if (e->timer == 60) {
			newobj->AddClassName("On2PKeySCGreat");
		}
		else if (e->timer == 61) {
			newobj->AddClassName("On2PKey1Great");
		}
		else if (e->timer == 62) {
			newobj->AddClassName("On2PKey2Great");
		}
		else if (e->timer == 63) {
			newobj->AddClassName("On2PKey3Great");
		}
		else if (e->timer == 64) {
			newobj->AddClassName("On2PKey4Great");
		}
		else if (e->timer == 65) {
			newobj->AddClassName("On2PKey5Great");
		}
		else if (e->timer == 66) {
			newobj->AddClassName("On2PKey6Great");
		}
		else if (e->timer == 67) {
			newobj->AddClassName("On2PKey7Great");
		}
		else if (e->timer == 68) {
			newobj->AddClassName("On2PKey8Great");
		}
		else if (e->timer == 69) {
			newobj->AddClassName("On2PKey9Great");
		}
		else if (e->timer == 70) {
			newobj->AddClassName("On1PKeySCHold");
		}
		else if (e->timer == 71) {
			newobj->AddClassName("On1PKey1Hold");
		}
		else if (e->timer == 72) {
			newobj->AddClassName("On1PKey2Hold");
		}
		else if (e->timer == 73) {
			newobj->AddClassName("On1PKey3Hold");
		}
		else if (e->timer == 74) {
			newobj->AddClassName("On1PKey4Hold");
		}
		else if (e->timer == 75) {
			newobj->AddClassName("On1PKey5Hold");
		}
		else if (e->timer == 76) {
			newobj->AddClassName("On1PKey6Hold");
		}
		else if (e->timer == 77) {
			newobj->AddClassName("On1PKey7Hold");
		}
		else if (e->timer == 78) {
			newobj->AddClassName("On1PKey8Hold");
		}
		else if (e->timer == 79) {
			newobj->AddClassName("On1PKey9Hold");
		}
		else if (e->timer == 80) {
			newobj->AddClassName("On2PKeySCHold");
		}
		else if (e->timer == 81) {
			newobj->AddClassName("On2PKey1Hold");
		}
		else if (e->timer == 82) {
			newobj->AddClassName("On2PKey2Hold");
		}
		else if (e->timer == 83) {
			newobj->AddClassName("On2PKey3Hold");
		}
		else if (e->timer == 84) {
			newobj->AddClassName("On2PKey4Hold");
		}
		else if (e->timer == 85) {
			newobj->AddClassName("On2PKey5Hold");
		}
		else if (e->timer == 86) {
			newobj->AddClassName("On2PKey6Hold");
		}
		else if (e->timer == 87) {
			newobj->AddClassName("On2PKey7Hold");
		}
		else if (e->timer == 88) {
			newobj->AddClassName("On2PKey8Hold");
		}
		else if (e->timer == 89) {
			newobj->AddClassName("On2PKey9Hold");
		}
		else if (e->timer == 100) {
			newobj->AddClassName("On1PKeySCPress");
		}
		else if (e->timer == 101) {
			newobj->AddClassName("On1PKey1Press");
		}
		else if (e->timer == 102) {
			newobj->AddClassName("On1PKey2Press");
		}
		else if (e->timer == 103) {
			newobj->AddClassName("On1PKey3Press");
		}
		else if (e->timer == 104) {
			newobj->AddClassName("On1PKey4Press");
		}
		else if (e->timer == 105) {
			newobj->AddClassName("On1PKey5Press");
		}
		else if (e->timer == 106) {
			newobj->AddClassName("On1PKey6Press");
		}
		else if (e->timer == 107) {
			newobj->AddClassName("On1PKey7Press");
		}
		else if (e->timer == 108) {
			newobj->AddClassName("On1PKey8Press");
		}
		else if (e->timer == 109) {
			newobj->AddClassName("On1PKey9Press");
		}
		else if (e->timer == 110) {
			newobj->AddClassName("On2PKeySCPress");
		}
		else if (e->timer == 111) {
			newobj->AddClassName("On2PKey1Press");
		}
		else if (e->timer == 112) {
			newobj->AddClassName("On2PKey2Press");
		}
		else if (e->timer == 113) {
			newobj->AddClassName("On2PKey3Press");
		}
		else if (e->timer == 114) {
			newobj->AddClassName("On2PKey4Press");
		}
		else if (e->timer == 115) {
			newobj->AddClassName("On2PKey5Press");
		}
		else if (e->timer == 116) {
			newobj->AddClassName("On2PKey6Press");
		}
		else if (e->timer == 117) {
			newobj->AddClassName("On2PKey7Press");
		}
		else if (e->timer == 118) {
			newobj->AddClassName("On2PKey8Press");
		}
		else if (e->timer == 119) {
			newobj->AddClassName("On2PKey9Press");
		}
		else if (e->timer == 120) {
			newobj->AddClassName("On1PKeySCUp");
		}
		else if (e->timer == 121) {
			newobj->AddClassName("On1PKey1Up");
		}
		else if (e->timer == 122) {
			newobj->AddClassName("On1PKey2Up");
		}
		else if (e->timer == 123) {
			newobj->AddClassName("On1PKey3Up");
		}
		else if (e->timer == 124) {
			newobj->AddClassName("On1PKey4Up");
		}
		else if (e->timer == 125) {
			newobj->AddClassName("On1PKey5Up");
		}
		else if (e->timer == 126) {
			newobj->AddClassName("On1PKey6Up");
		}
		else if (e->timer == 127) {
			newobj->AddClassName("On1PKey7Up");
		}
		else if (e->timer == 128) {
			newobj->AddClassName("On1PKey8Up");
		}
		else if (e->timer == 129) {
			newobj->AddClassName("On1PKey9Up");
		}
		else if (e->timer == 130) {
			newobj->AddClassName("On2PKeySCUp");
		}
		else if (e->timer == 131) {
			newobj->AddClassName("On2PKey1Up");
		}
		else if (e->timer == 132) {
			newobj->AddClassName("On2PKey2Up");
		}
		else if (e->timer == 133) {
			newobj->AddClassName("On2PKey3Up");
		}
		else if (e->timer == 134) {
			newobj->AddClassName("On2PKey4Up");
		}
		else if (e->timer == 135) {
			newobj->AddClassName("On2PKey5Up");
		}
		else if (e->timer == 136) {
			newobj->AddClassName("On2PKey6Up");
		}
		else if (e->timer == 137) {
			newobj->AddClassName("On2PKey7Up");
		}
		else if (e->timer == 138) {
			newobj->AddClassName("On2PKey8Up");
		}
		else if (e->timer == 139) {
			newobj->AddClassName("On2PKey9Up");
		}
		else if (e->timer == 140) {
			newobj->AddClassName("OnBeat");
		}

		// fill information to object
		newobj->GetDstArray() = e->dst;
		newobj->SetTag(e->objname);
		newobj->looptime_dst = e->looptime;
		newobj->blend = e->blend;
		newobj->rotatecenter = e->rotatecenter;

		// process src 
		ImageSRC src;
		if (e->divx <= 0) e->divx = 1;
		if (e->divy <= 0) e->divy = 1;
		newobj->divx = e->divx;
		newobj->divy = e->divy;
		newobj->src = e->src;
		newobj->cycle = e->cycle;

		return newobj;
	}
	else {
		/*
		 * ohh... it's not a valid object...
		 */
		printf("[WARNING] Unknown Object(%s). ignored. (%dL)\n", e->objname, current_line);
		return 0;
	}
}

void _LR2SkinParser::SetSkinOption(int idx) {
	current_skin_option[idx] = true;
}

bool _LR2SkinParser::CheckOption(int option) {
	for (int i = 0; i < parser_condition.size(); i++)
	{
		if (parser_condition[i] == option)
			return true;
	}
	return false;
}

void _LR2SkinParser::Clear() {
	option_id_name.clear();
	option_fn_name.clear();
	parser_condition.clear();
	prv_obj = 0;
}

// ------------------------ LR2Skin Element part ---------------------------

void _LR2SkinElement::AddSrc(char **args) {
	resource_id = INT(args[1]);
	timer_src = INT(args[9]);
	divx = INT(args[6]);
	divy = INT(args[7]);
	cycle = INT(args[8]);	// cycle

	src.x = INT(args[2]);
	src.y = INT(args[3]);
	src.w = INT(args[4]);
	src.h = INT(args[5]);
}

void _LR2SkinElement::AddDst(char **args) {
	ImageDST dst;
	dst.time = INT(args[1]);
	dst.x = INT(args[2]);
	dst.y = INT(args[3]);
	dst.w = INT(args[4]);
	dst.h = INT(args[5]);
	dst.acc = INT(args[6]);
	dst.a = INT(args[7]);
	dst.r = INT(args[8]);
	dst.g = INT(args[9]);
	dst.b = INT(args[10]);
	dst.angle = INT(args[13]);
	if (this->dst.size() == 0) {
		blend = INT(args[11]);
		usefilter = INT(args[12]);
		center = INT(args[14]);
		looptime = INT(args[15]);
		timer = INT(args[16]);

		if (INT(args[17])) options.push_back(INT(args[17]));
		if (INT(args[18])) options.push_back(INT(args[18]));
		if (INT(args[19])) options.push_back(INT(args[19]));
	}
	this->dst.push_back(dst);
}

bool _LR2SkinElement::IsValid() {
	return (dst.size() > 0);
}

bool _LR2SkinElement::CheckOption(int option) {
	for (int i = 0; i < options.size(); i++)
	{
		if (options[i] == option)
			return true;
	}
	return false;
}

void _LR2SkinElement::CopyOptions(std::vector<int>& opts) {
	options = opts;
}

void _LR2SkinElement::Clear() {
	//src.clear();
	dst.clear();
	options.clear();
}

// ----------------------- LR2Skin part end ------------------------

bool Skin::Parse(const char *filepath) {
	// TODO
	printf("[WARNING] you called not implemented function!\n");
	return false;
}

void Skin::AddElement(SkinElement *e) {
	// copy object and add
	SkinElement *ne = new SkinElement(*e);
	skinelement_all.AddElement(ne);
	// register id
	if (skinelement_id.find(ne->id) == skinelement_id.end()) {
		skinelement_id.insert(std::pair<std::string, SkinElementGroup>(ne->id, SkinElementGroup(ne)));
	}
	else {
		skinelement_id[ne->id].AddElement(ne);
	}
	// register classname
	for (auto it = ne->classnames.begin(); it != ne->classnames.end(); ++it) {
		if (skinelement_class.find(*it) == skinelement_class.end()) {
			skinelement_class.insert(std::pair<std::string, SkinElementGroup>(*it, SkinElementGroup(ne)));
		}
		else {
			skinelement_class[*it].AddElement(ne);
		}
	}
	// TODO: register child/parent.
}

void Skin::Release() {
	skinelement_id.clear();
	skinelement_class.clear();
	for (auto it = skinelement_all.GetAllElements().begin(); it != skinelement_all.GetAllElements().end(); ++it) {
		delete *it;
	}
}

Skin::Skin() {}
Skin::~Skin() { Release(); }

// --------------------- Skin End --------------------------

void SkinResource::LoadResource(Skin &s) {
	// search image part
	std::vector<std::string> &imgs_header = s.resource_imgs;
	std::vector<SkinFont> &fonts_header = s.resource_fonts;
	int i;
	
	// image load
	i= 0;
	for (auto it = imgs_header.begin(); it != imgs_header.end(); ++it) {
		if (!imgs[i].Load((*it)))
			printf("Failed to load skin resource - %s\n", (*it).c_str());
		i++;
	}
	
	// font load
	// if cannot load font/texture, then load fallback font/texture
	// TODO: texture

	i = 0;
	for (auto it = fonts_header.begin(); it != fonts_header.end(); ++it) {
		if (!(fonts[i] = FC_CreateFont())) {
			printf("Failed to initalize font.\n");
			continue;
		}
		if (!FC_LoadFont(fonts[i], Game::GetRenderer(), (*it).filepath.c_str(),
			(*it).fontsize, SDL_Color(), (*it).style)) {
			printf("Failed to load font resource - %s\n", (*it).filepath.c_str());
			FC_LoadFont(fonts[i], Game::GetRenderer(), "../skin/_default/NanumGothic.ttf",
				(*it).fontsize, SDL_Color(), (*it).style);
		}
		i++;
	}
}

Image* SkinResource::GetImageResource(int idx) {
	return &imgs[idx];
}

void SkinResource::Release() {
	for (int i = 0; i < 50; i++) if (imgs[i].IsLoaded()) {
		imgs[i].Release();
		if (fonts[i])
			FC_FreeFont(fonts[i]);
	}
}

SkinResource::~SkinResource() {
	Release();
}

// ------------------ SkinResource End ---------------------

