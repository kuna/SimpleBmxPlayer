#include "skin.h"
#include "skinutil.h"
using namespace SkinUtil;

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

bool _LR2SkinParser::ParseLR2Skin(const char *filepath, Skin *s) {
	this->s = s;

	// fill basic information to skin
	strcpy(s->filepath, filepath);
	/* 4 elements are necessary */
	XMLElement *info = s->skinlayout.NewElement("Info");
	XMLElement *resource = s->skinlayout.NewElement("Resource");
	XMLElement *skin = s->skinlayout.NewElement("Skin");
	XMLElement *option = s->skinlayout.NewElement("Option");
	s->skinlayout.LinkEndChild(info);
	s->skinlayout.LinkEndChild(option);
	s->skinlayout.LinkEndChild(resource);
	s->skinlayout.LinkEndChild(skin);

	// load skin line
	// because lr2skin file format has no end tag, 
	//  it's better to load all lines before parsing
	line_total = LoadSkin(filepath);

	// after we read all lines, skin parse start
	condition_element[0] = cur_e = skin;
	condition_level = 0;
	ParseSkin();

	// release all used data
	Clear();

	if (cur_e == s->skinlayout.ToElement()) {
		printf("lr2skin(%s) parsing successfully done\n", filepath);
		return true;
	}
	else {
		// if this value is the final output,
		// then there might be some #ENDIF was leaked.
		return false;
	}
}

int _LR2SkinParser::LoadSkin(const char *filepath, int linebufferpos) {
	FILE *f = fopen(filepath, "r");
	if (!f) {
		printf("[ERROR] Cannot find Skin file %s - ignore\n", filepath);
		return linebufferpos;
	}

	char line[10240];
	char *p;
	int current_line = 0;		// current file's reading line
	while (!feof(f)) {
		current_line++;
		fgets(line, 1024, f);
		p = Trim(line);

		// ignore comment
		if (strncmp("//", p, 2) == 0 || !strlen(p))
			continue;

		// if #include then read the file first
		if (strncmp("#INCLUDE", p, 8) == 0) {
			linebufferpos += LoadSkin(p + 9, linebufferpos + 1);
		}
		else {
			strcpy(lines[linebufferpos], line);
			ParseSkinLineArgument(lines[linebufferpos], line_args[linebufferpos]);
			line_position[linebufferpos] = current_line;
		}

		linebufferpos++;
	}

	fclose(f);

	// returns how much lines we read
	return linebufferpos;
}

void _LR2SkinParser::ParseSkin() {
	// start first line to end line
	int current_line = 0;
	while (current_line >= 0 && current_line < line_total) {
		current_line = ParseSkinLine(current_line);
	}
}

void _LR2SkinParser::ParseSkinLineArgument(char *p, char **args) {
	// first element is element name
	args[0] = p;
	int i;
	for (i = 1; i < 50; i++) {
		p = strchr(p, ',');
		if (!p)
			break;
		*p = 0;
		args[i] = (++p);
	}
	// for safety, pack last argument to 0
	args[i] = 0;
}

int _LR2SkinParser::ParseSkinLine(int line) {
	// get current line's string & argument
	char **args;			// contains linebuffer's address
	args = line_args[line];
	if (args[0] == 0)
		return line + 1;

#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
#define OBJTYPE_IS(v) (strcmp(args[0]+5, (v)) == 0)
#define CMD_STARTSWITH(v,l) (strncmp(args[0], (v), (l)) == 0)
#define INT(v) (atoi(v))
#define ADDTOHEADER(v) (v)
	/*
	* condition part
	*/
	if (CMD_IS("#ENDIF")) {
		// we just ignore this statement, so only 1st-level parsing is enabled.
		// but if previous #IF clause exists, then close it
		cur_e = condition_element[--condition_level];
		return line + 1;
	}
	if (!cur_e)
		return line + 1;
	if (CMD_IS("#ELSE")) {
		// we tend to ignore #ELSE clause ... (nothing we can process about it)
		cur_e = 0;
		return line + 1;
	}
	else if (CMD_IS("#IF") || CMD_IS("#ELSEIF")) {
		// #ELSEIF: think as new #IF clause
		XMLElement *group = s->skinlayout.NewElement("Group");
		if (CMD_IS("#IF")) condition_level++;
		condition_element[condition_level] = group;
		ClassAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = TranslateOPs(INT(args[i]));
			if (c) cls.AddClass(c);
		}
		group->SetAttribute("class", cls.ToString());
		cur_e = condition_element[condition_level - 1];		// get parent object
		cur_e->LinkEndChild(group);
		cur_e = group;
		return line + 1;
	}

	/*
	* header/metadata parsing
	*/
	if (CMD_IS("#CUSTOMOPTION")) {
		XMLElement *customoption = s->skinlayout.NewElement("CustomOption");
		customoption->SetAttribute("name", args[1]);
		int option_intvalue = atoi(args[2]);
		for (char **p = args + 3; *p != 0 && strlen(*p) > 0; p++) {
			XMLElement *options = s->skinlayout.NewElement("Option");
			options->SetAttribute("name", *p);
			options->SetAttribute("class", option_intvalue);
			option_intvalue++;
			customoption->LinkEndChild(options);
		}
		XMLElement *option = s->skinlayout.FirstChildElement("Option");
		option->LinkEndChild(customoption);
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		XMLElement *customfile = s->skinlayout.NewElement("CustomFile");
		customfile->SetAttribute("name", args[1]);
		customfile->SetAttribute("filter", args[2]);
		customfile->SetAttribute("default", args[3]);
		XMLElement *option = s->skinlayout.FirstChildElement("Option");
		option->LinkEndChild(customfile);
	}
	else if (CMD_IS("#INFORMATION")) {
		// set skin's metadata
		XMLElement *info = s->skinlayout.FirstChildElement("Info");
		XMLElement *type = s->skinlayout.NewElement("Type");
		int type_ = INT(args[1]);		// 0: 7key, 1: 9key, 2: 14key, 12: battle
		switch (type_) {
		case 0:
			// 7key
			type->SetText("7Key");
			break;
		case 1:
			// 9key
			type->SetText("9Key");
			break;
		case 2:
			// 14key
			type->SetText("14Key");
			break;
		case 12:
			// battle
			type->SetText("7KBattle");
			break;
		default:
			printf("[ERROR] unknown type of lr2skin(%d). consider as 7Key.\n", type_);
		}
		XMLElement *skinname = s->skinlayout.NewElement("Name");
		skinname->SetText(args[2]);
		XMLElement *author = s->skinlayout.NewElement("Author");
		author->SetText(args[3]);
		info->LinkEndChild(type);
		info->LinkEndChild(skinname);
		info->LinkEndChild(author);
	}
	/*
	* we very first parsed #INCLUDE command
	* so we don't need to process it
	*
	else if (CMD_IS("#INCLUDE")) {
	Parse(args[1]);
	}*/
	else if (CMD_IS("#IMAGE")) {
		XMLElement *resource = s->skinlayout.FirstChildElement("Resource");
		XMLElement *image = s->skinlayout.NewElement("Image");
		image->SetAttribute("path", args[1]);
		resource->LinkEndChild(image);
	}
	else if (CMD_IS("#FONT")) {
		// we don't use bitmap fonts
		// So if cannot found, we'll going to use default font/texture.
		// current font won't support TTF, so basically we're going to use default font.
		XMLElement *resource = s->skinlayout.FirstChildElement("Resource");
		XMLElement *font = s->skinlayout.NewElement("Font");
		font->SetAttribute("ttfpath", args[4]);
		font->SetAttribute("texturepath", "");
		font->SetAttribute("size", INT(args[1]));
		switch (INT(args[3])) {
		case 0:
			// normal
			font->SetAttribute("style", "normal");
			break;
		case 1:
			// italic
			font->SetAttribute("style", "italic");
			break;
		case 2:
			// bold
			font->SetAttribute("style", "bold");
			break;
		}
		font->SetAttribute("thickness", INT(args[2]));
		font->SetAttribute("border", 1);
		resource->LinkEndChild(font);
	}
	else if (CMD_IS("#SETOPTION")) {
		// this clause is translated during render tree construction
		XMLElement *setoption = s->skinlayout.NewElement("SetOption");
		ClassAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = TranslateOPs(INT(args[i]));
			if (c)
				cls.AddClass(c);
		}
		setoption->SetAttribute("class", cls.ToString());
		cur_e->LinkEndChild(setoption);
	}
	/* Just ignore these option */
	else if (CMD_IS("#ENDOFHEADER")) {}
	else if (CMD_IS("#FLIPRESULT")) {}
	else if (CMD_IS("#TRANSCLOLR")) {}		// should we have to implement colorkey?
	else if (CMD_STARTSWITH("#SRC_", 4)){
		// we parse #DST with #SRC.
		// process SRC
		XMLElement *obj;
		XMLElement *src = s->skinlayout.NewElement("SRC");
		src->SetAttribute("id", INT(args[2]));
		src->SetAttribute("x", INT(args[3]));
		src->SetAttribute("y", INT(args[4]));
		if (INT(args[5]) > 0) {
			src->SetAttribute("w", INT(args[5]));
			src->SetAttribute("h", INT(args[6]));
		}
		if (INT(args[7]) > 1 || INT(args[8]) > 1) {
			src->SetAttribute("divx", INT(args[7]));
			src->SetAttribute("divy", INT(args[8]));
		}
		if (INT(args[9]))
			src->SetAttribute("cycle", INT(args[9]));
		int sop1 = 0, sop2 = 0, sop3 = 0;
		if (args[11]) sop1 = INT(args[11]);
		if (args[12]) sop2 = INT(args[12]);
		if (args[13]) sop3 = INT(args[13]);

		/*
		 * process NOT-general-objects first
		 * these objects doesn't have #DST object directly
		 * (bad-syntax >:( )
		 */
		// check for play area
		int isPlayElement = ProcessLane(src, line);
		if (isPlayElement) {
			return isPlayElement;
		}

		/*
		 * under these are objects which requires #DST object directly
		 * (good syntax)
		 * we have to make object now to parse #DST
		 * and, if unable to figure out what this element is, it'll be considered as Image object.
		 */
		obj = s->skinlayout.NewElement("Image");
		obj->LinkEndChild(src);
		cur_e->LinkEndChild(obj);

		int looptime = 0, blend = 0, timer = 0, rotatecenter = -1;
		int op1 = 0, op2 = 0, op3 = 0;
		int nl;
		for (nl = line + 1; nl < line_total; nl++) {
			args = line_args[nl];
			if (!args[0]) continue;
			if (!CMD_STARTSWITH("#DST_", 5))
				break;
			// if it's very first line (parse basic element)
			if (rotatecenter < 0) {
				looptime = INT(args[16]);
				blend = INT(args[12]);
				rotatecenter = INT(args[15]);
				timer = INT(args[17]);
				if (args[18]) op1 = INT(args[18]);
				if (args[19]) op2 = INT(args[19]);
				if (args[20]) op3 = INT(args[20]);
			}
			XMLElement *dst = s->skinlayout.NewElement("DST");
			dst->SetAttribute("x", args[3]);
			dst->SetAttribute("y", args[4]);
			dst->SetAttribute("w", args[5]);
			dst->SetAttribute("h", args[6]);
			dst->SetAttribute("time", args[2]);
			if (!(INT(args[8]) == 255))
				dst->SetAttribute("a", args[8]);
			if (!(INT(args[9]) == 255 && INT(args[10]) == 255 && INT(args[11]) == 255)) {
				dst->SetAttribute("r", args[9]);
				dst->SetAttribute("g", args[10]);
				dst->SetAttribute("b", args[11]);
			}
			if (INT(args[14]))
				dst->SetAttribute("angle", args[14]);
			if (INT(args[2]) == looptime)
				dst->SetAttribute("loop", true);
			obj->LinkEndChild(dst);
		}
		// set common draw attribute
		if (blend > 1)
			obj->SetAttribute("blend", blend);
		if (rotatecenter > 0)
			obj->SetAttribute(
			"rotatecenter", rotatecenter);
		if (TranslateTimer(timer))
			obj->SetAttribute("timer", TranslateTimer(timer));
		// before register, check loop statement (is loop is in last object, it isn't necessary)
		if (obj->LastChild()->ToElement()->Attribute("loop")) {
			obj->LastChild()->ToElement()->DeleteAttribute("loop");
		}
		

		/*
		 * Check out for some special object (which requires #DST object)
		 * COMMENT: most of them behaves like #IMAGE object.
		 */
		// reset arguments to figure out about object
		args = line_args[line];
		int objectid = INT(args[1]);

		// groovegauge
		if (OBJTYPE_IS("GROOVEGAUGE")) {
			obj->SetName("LifeGraph");
			obj->SetAttribute("player", objectid);
			obj->SetAttribute("valueid", "Life");
			return line + 1;
		}

		// combo menu
		int isComboElement = ProcessCombo(obj, line);
		if (isComboElement)
			return isComboElement;

		// select menu part
		if (OBJTYPE_IS("BAR_BODY")) {
		}
		else if (OBJTYPE_IS("BAR_FLASH")) {
		}
		else if (OBJTYPE_IS("BAR_TITLE")) {
		}
		else if (OBJTYPE_IS("BAR_LEVEL")) {
		}
		else if (OBJTYPE_IS("BAR_LAMP")) {
		}
		else if (OBJTYPE_IS("BAR_MY_LAMP")) {
		}
		else if (OBJTYPE_IS("BAR_RIVAL_LAMP")) {
		}
		else if (OBJTYPE_IS("BAR_RIVAL")) {
		}
		else if (OBJTYPE_IS("BAR_RANK")) {
		}

		/* 
		 * under these are general individual object
		 */
		if (OBJTYPE_IS("IMAGE")) {
			// set class & common option
			// set DST & common draw attribute
			ClassAttribute cls;
			const char *c;
			c = TranslateOPs(op1);
			if (c) cls.AddClass(c);
			c = TranslateOPs(op2);
			if (c) cls.AddClass(c);
			c = TranslateOPs(op3);
			if (c) cls.AddClass(c);
			if (cls.GetClassNumber())
				obj->SetAttribute("class", cls.ToString());
		}
		else if (OBJTYPE_IS("BGA")) {
			// change tag to BGA and remove SRC tag
			s->skinlayout.DeleteNode(src);
			obj->SetName("Bga");
		}
		else if (OBJTYPE_IS("NUMBER")) {
			// change tag to Number and add attr
			obj->SetName("Number");
			obj->SetAttribute("value", sop1);
			obj->SetAttribute("align", sop2);
			obj->SetAttribute("length", sop3);
		}
		else if (OBJTYPE_IS("SLIDER")) {
			// change tag to slider and add attr
			obj->SetName("Slider");
			obj->SetAttribute("direction", sop1);
			obj->SetAttribute("range", sop2);
			obj->SetAttribute("value", sop3);
			//obj->SetAttribute("range", sop2); - disable option is ignored
		}
		else if (OBJTYPE_IS("TEXT")) {
			// delete src object and change to font/st/align/edit
			s->skinlayout.DeleteNode(src);
			obj->SetName("Text");
			obj->SetAttribute("font", args[2]);
			obj->SetAttribute("value", args[3]);
			obj->SetAttribute("align", args[4]);
			obj->SetAttribute("edit", args[5]);
		}
		else if (OBJTYPE_IS("BARGRAPH")) {
			obj->SetName("BarGraph");
			obj->SetAttribute("value", sop1);
			obj->SetAttribute("direction", sop2);
		}
		else if (OBJTYPE_IS("BUTTON")) {
			// TODO: onhover ...?
			obj->SetName("Button");
		}
		else {
			printf("Unknown General Object (%s), consider as IMAGE. (%dL)\n", args[0] + 5, line_position[line]);
		}

		// return new line
		return nl;
	}
	else if (CMD_STARTSWITH("#DST_BAR_BODY", 13)) {
		// select menu part
		if (CMD_IS("#DST_BAR_BODY_ON")) {

		}
		else if (CMD_IS("#DST_BAR_BODY_OFF")) {

		}
	}
	else if (CMD_STARTSWITH("#DST_", 5)) {
		// just ignore
	}
	else {
		printf("Unknown Type: %s (%dL) - Ignore.\n", args[0], line_position[line]);
	}
#undef CMD_IS

	// parse next line
	return line + 1;
}

/*
 * process commands about lane
 * if not lane, return 0
 * if lane, return next parsed line
 */
int _LR2SkinParser::ProcessLane(XMLElement *src, int line) {
	char **args = line_args[line];
	int objectid = INT(args[1]);

	if (OBJTYPE_IS("NOTE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_NOTE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_END")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_END");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_BODY")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_BODY");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_START")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_END");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("MINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_MINE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	if (OBJTYPE_IS("AUTO_NOTE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_NOTE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_END")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_END");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_BODY")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_BODY");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_START")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_START");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_MINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Lane", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_MINE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LINE");
		playarea->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("JUDGELINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid, &s->skinlayout);
		// find DST object to set Lane attribute
		for (int _l = line + 1; _l < line_total; _l++) {
			if (line_args[_l][0] && strcmp(line_args[_l][0], "#DST_JUDGELINE") == 0 && INT(line_args[_l][1]) == objectid) {
				playarea->SetAttribute("x", INT(line_args[_l][3]));
				playarea->SetAttribute("y", 0);
				playarea->SetAttribute("w", INT(line_args[_l][5]));
				playarea->SetAttribute("h", INT(line_args[_l][4]));
				break;
			}
		}
		// add src to here
		src->SetName("SRC_JUDGELINE");
		playarea->LinkEndChild(src);
		return line + 1;
	}

	// not an play object
	return 0;
}

int _LR2SkinParser::ProcessCombo(XMLElement *obj, int line) {
	char **args = line_args[line];
	int objectid = INT(args[1]);

	if (OBJTYPE_IS("NOWJUDGE_1P")) {
		XMLElement *playcombo = FindElementWithAttribute(cur_e, "PlayCombo", "side", 0, &s->skinlayout);
		obj->SetName("NowJudge");
		obj->SetAttribute("level", objectid);
		playcombo->LinkEndChild(obj);
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_1P")) {
		XMLElement *playcombo = FindElementWithAttribute(cur_e, "PlayCombo", "side", 0, &s->skinlayout);
		obj->SetName("NowCombo");
		obj->SetAttribute("level", objectid);
		playcombo->LinkEndChild(obj);
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWJUDGE_2P")) {
		XMLElement *playcombo = FindElementWithAttribute(cur_e, "PlayCombo", "side", 1, &s->skinlayout);
		obj->SetName("NowJudge");
		obj->SetAttribute("level", objectid);
		playcombo->LinkEndChild(obj);
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_2P")) {
		XMLElement *playcombo = FindElementWithAttribute(cur_e, "PlayCombo", "side", 1, &s->skinlayout);
		obj->SetName("NowCombo");
		obj->SetAttribute("level", objectid);
		playcombo->LinkEndChild(obj);
		return line + 1;
	}

	// not a combo object
	return 0;
}

/*
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
// Sudden/Lift, all of them are setted by judgeline's xywh,
// So we ignore all of the dst option of this slider.
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

// we're figured out it's a valid object
// and decided to make object solid
if (newobj) {
// make classname from conditions
// TODO: Skin option effect - how can we process it?

// make classname from timer

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
// ohh... it's not a valid object...
printf("[WARNING] Unknown Object(%s). ignored. (%dL)\n", e->objname, current_line);
return 0;
}
}
*/
const char* _LR2SkinParser::TranslateOPs(int op) {
	if (op == 1) {
		strcpy(translated, "OnClose");
	}
	else if (op == 10) {
		strcpy(translated, "DoublePlay");
	}
	else if (op == 11) {
		strcpy(translated, "BattlePlay");
	}
	else if (op == 12) {
		strcpy(translated, "DoublePlay");	// this includes battle
	}
	else if (op == 13) {
		strcpy(translated, "BattlePlay");	// this includes ghost battle
	}
	else if (op == 32) {
		strcpy(translated, "AutoPlay");
	}
	else if (op == 33) {
		strcpy(translated, "AutoPlayOff");
	}
	else if (op == 34) {
		strcpy(translated, "GhostOff");
	}
	else if (op == 35) {
		strcpy(translated, "GhostA");
	}
	else if (op == 36) {
		strcpy(translated, "GhostB");
	}
	else if (op == 37) {
		strcpy(translated, "GhostC");
	}
	else if (op == 38) {
		strcpy(translated, "ScoreGraphOff");
	}
	else if (op == 39) {
		strcpy(translated, "ScoreGraph");
	}
	else if (op == 40) {
		strcpy(translated, "BGAOff");
	}
	else if (op == 41) {
		strcpy(translated, "BGAOn");
	}
	else if (op == 42) {
		strcpy(translated, "1PGrooveGuage");
	}
	else if (op == 43) {
		strcpy(translated, "1PHardGuage");
	}
	else if (op == 44) {
		strcpy(translated, "2PGrooveGuage");
	}
	else if (op == 45) {
		strcpy(translated, "2PHardGuage");
	}
	else if (op == 46) {
		strcpy(translated, "2PHardGuage");
	}
	else if (op == 47) {
		strcpy(translated, "2PHardGuage");
	}
	else if (op == 50) {
		strcpy(translated, "Offline");
	}
	else if (op == 51) {
		strcpy(translated, "Online");
	}
	else if (op == 80) {
		strcpy(translated, "OnSongLoadingStart");
	}
	else if (op == 81) {
		strcpy(translated, "OnSongLoadingEnd");
	}
	else if (op == 150) {
		strcpy(translated, "DiffNone");
	}
	else if (op == 151) {
		strcpy(translated, "DiffBeginner");
	}
	else if (op == 152) {
		strcpy(translated, "DiffNormal");
	}
	else if (op == 153) {
		strcpy(translated, "DiffHyper");
	}
	else if (op == 154) {
		strcpy(translated, "DiffAnother");
	}
	else if (op == 155) {
		strcpy(translated, "DiffInsane");
	}
	else if (op == 270) {
		strcpy(translated, "On1PSuddenChange");
	}
	else if (op == 271) {
		strcpy(translated, "On2PSuddenChange");
	}
	else if (op == 292) {
		strcpy(translated, "ExpertCourse");
	}
	else if (op == 293) {
		strcpy(translated, "ClassCourse");
	}
	else if (op >= 900)
	{
		// #CUSTOMOPTION code
		itoa(op, translated, 10);
	}
	else {
		// unknown!
		return 0;
	}
	return translated;
}

const char* _LR2SkinParser::TranslateTimer(int timer) {
	if (timer == 2) {
		strcpy(translated, "OnClose");	// FADEOUT
	}
	else if (timer == 3) {
		strcpy(translated, "OnFail");	// Stage failed
	}
	else if (timer == 40) {
		strcpy(translated, "OnReady");
	}
	else if (timer == 41) {
		strcpy(translated, "OnGameStart");
	}
	else if (timer == 42) {
		strcpy(translated, "On1PGuageUp");
	}
	else if (timer == 43) {
		strcpy(translated, "On2PGuageUp");
	}
	else if (timer == 44) {
		strcpy(translated, "On1PGuageMax");
	}
	else if (timer == 45) {
		strcpy(translated, "On2PGuageMax");
	}
	else if (timer == 46) {
		strcpy(translated, "On1PJudge");
	}
	else if (timer == 47) {
		strcpy(translated, "On2PJudge");
	}
	else if (timer == 48) {
		strcpy(translated, "On1PFullCombo");
	}
	else if (timer == 49) {
		strcpy(translated, "On2PFullCombo");
	}
	else if (timer == 50) {
		strcpy(translated, "On1PKeySCGreat");
	}
	else if (timer == 51) {
		strcpy(translated, "On1PKey1Great");
	}
	else if (timer == 52) {
		strcpy(translated, "On1PKey2Great");
	}
	else if (timer == 53) {
		strcpy(translated, "On1PKey3Great");
	}
	else if (timer == 54) {
		strcpy(translated, "On1PKey4Great");
	}
	else if (timer == 55) {
		strcpy(translated, "On1PKey5Great");
	}
	else if (timer == 56) {
		strcpy(translated, "On1PKey6Great");
	}
	else if (timer == 57) {
		strcpy(translated, "On1PKey7Great");
	}
	else if (timer == 58) {
		strcpy(translated, "On1PKey8Great");
	}
	else if (timer == 59) {
		strcpy(translated, "On1PKey9Great");
	}
	else if (timer == 60) {
		strcpy(translated, "On2PKeySCGreat");
	}
	else if (timer == 61) {
		strcpy(translated, "On2PKey1Great");
	}
	else if (timer == 62) {
		strcpy(translated, "On2PKey2Great");
	}
	else if (timer == 63) {
		strcpy(translated, "On2PKey3Great");
	}
	else if (timer == 64) {
		strcpy(translated, "On2PKey4Great");
	}
	else if (timer == 65) {
		strcpy(translated, "On2PKey5Great");
	}
	else if (timer == 66) {
		strcpy(translated, "On2PKey6Great");
	}
	else if (timer == 67) {
		strcpy(translated, "On2PKey7Great");
	}
	else if (timer == 68) {
		strcpy(translated, "On2PKey8Great");
	}
	else if (timer == 69) {
		strcpy(translated, "On2PKey9Great");
	}
	else if (timer == 70) {
		strcpy(translated, "On1PKeySCHold");
	}
	else if (timer == 71) {
		strcpy(translated, "On1PKey1Hold");
	}
	else if (timer == 72) {
		strcpy(translated, "On1PKey2Hold");
	}
	else if (timer == 73) {
		strcpy(translated, "On1PKey3Hold");
	}
	else if (timer == 74) {
		strcpy(translated, "On1PKey4Hold");
	}
	else if (timer == 75) {
		strcpy(translated, "On1PKey5Hold");
	}
	else if (timer == 76) {
		strcpy(translated, "On1PKey6Hold");
	}
	else if (timer == 77) {
		strcpy(translated, "On1PKey7Hold");
	}
	else if (timer == 78) {
		strcpy(translated, "On1PKey8Hold");
	}
	else if (timer == 79) {
		strcpy(translated, "On1PKey9Hold");
	}
	else if (timer == 80) {
		strcpy(translated, "On2PKeySCHold");
	}
	else if (timer == 81) {
		strcpy(translated, "On2PKey1Hold");
	}
	else if (timer == 82) {
		strcpy(translated, "On2PKey2Hold");
	}
	else if (timer == 83) {
		strcpy(translated, "On2PKey3Hold");
	}
	else if (timer == 84) {
		strcpy(translated, "On2PKey4Hold");
	}
	else if (timer == 85) {
		strcpy(translated, "On2PKey5Hold");
	}
	else if (timer == 86) {
		strcpy(translated, "On2PKey6Hold");
	}
	else if (timer == 87) {
		strcpy(translated, "On2PKey7Hold");
	}
	else if (timer == 88) {
		strcpy(translated, "On2PKey8Hold");
	}
	else if (timer == 89) {
		strcpy(translated, "On2PKey9Hold");
	}
	else if (timer == 100) {
		strcpy(translated, "On1PKeySCPress");
	}
	else if (timer == 101) {
		strcpy(translated, "On1PKey1Press");
	}
	else if (timer == 102) {
		strcpy(translated, "On1PKey2Press");
	}
	else if (timer == 103) {
		strcpy(translated, "On1PKey3Press");
	}
	else if (timer == 104) {
		strcpy(translated, "On1PKey4Press");
	}
	else if (timer == 105) {
		strcpy(translated, "On1PKey5Press");
	}
	else if (timer == 106) {
		strcpy(translated, "On1PKey6Press");
	}
	else if (timer == 107) {
		strcpy(translated, "On1PKey7Press");
	}
	else if (timer == 108) {
		strcpy(translated, "On1PKey8Press");
	}
	else if (timer == 109) {
		strcpy(translated, "On1PKey9Press");
	}
	else if (timer == 110) {
		strcpy(translated, "On2PKeySCPress");
	}
	else if (timer == 111) {
		strcpy(translated, "On2PKey1Press");
	}
	else if (timer == 112) {
		strcpy(translated, "On2PKey2Press");
	}
	else if (timer == 113) {
		strcpy(translated, "On2PKey3Press");
	}
	else if (timer == 114) {
		strcpy(translated, "On2PKey4Press");
	}
	else if (timer == 115) {
		strcpy(translated, "On2PKey5Press");
	}
	else if (timer == 116) {
		strcpy(translated, "On2PKey6Press");
	}
	else if (timer == 117) {
		strcpy(translated, "On2PKey7Press");
	}
	else if (timer == 118) {
		strcpy(translated, "On2PKey8Press");
	}
	else if (timer == 119) {
		strcpy(translated, "On2PKey9Press");
	}
	else if (timer == 120) {
		strcpy(translated, "On1PKeySCUp");
	}
	else if (timer == 121) {
		strcpy(translated, "On1PKey1Up");
	}
	else if (timer == 122) {
		strcpy(translated, "On1PKey2Up");
	}
	else if (timer == 123) {
		strcpy(translated, "On1PKey3Up");
	}
	else if (timer == 124) {
		strcpy(translated, "On1PKey4Up");
	}
	else if (timer == 125) {
		strcpy(translated, "On1PKey5Up");
	}
	else if (timer == 126) {
		strcpy(translated, "On1PKey6Up");
	}
	else if (timer == 127) {
		strcpy(translated, "On1PKey7Up");
	}
	else if (timer == 128) {
		strcpy(translated, "On1PKey8Up");
	}
	else if (timer == 129) {
		strcpy(translated, "On1PKey9Up");
	}
	else if (timer == 130) {
		strcpy(translated, "On2PKeySCUp");
	}
	else if (timer == 131) {
		strcpy(translated, "On2PKey1Up");
	}
	else if (timer == 132) {
		strcpy(translated, "On2PKey2Up");
	}
	else if (timer == 133) {
		strcpy(translated, "On2PKey3Up");
	}
	else if (timer == 134) {
		strcpy(translated, "On2PKey4Up");
	}
	else if (timer == 135) {
		strcpy(translated, "On2PKey5Up");
	}
	else if (timer == 136) {
		strcpy(translated, "On2PKey6Up");
	}
	else if (timer == 137) {
		strcpy(translated, "On2PKey7Up");
	}
	else if (timer == 138) {
		strcpy(translated, "On2PKey8Up");
	}
	else if (timer == 139) {
		strcpy(translated, "On2PKey9Up");
	}
	else if (timer == 140) {
		strcpy(translated, "OnBeat");
	}
	else {
		// unknown timer!
		return 0;
	}
	return translated;
}

void _LR2SkinParser::Clear() {
	line_total = 0;
	cur_e = 0;
}

// ----------------------- LR2Skin part end ------------------------
