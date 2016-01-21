#include "skin.h"
#include "skinutil.h"
#include "skintexturefont.h"
using namespace SkinUtil;
using namespace tinyxml2;
#include <sstream>

// temp
char translated[1024];

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
	// release all used data before starting
	Clear();

	// fill basic information to skin
	this->s = s;

	strcpy(s->filepath, filepath);
	XMLComment *cmt = s->skinlayout.NewComment("Auto-generated code.\n"
		"Converted from LR2Skin.\n"
		"LR2 project origins from lavalse, All rights reserved.");
	s->skinlayout.LinkEndChild(cmt);
	/* 4 elements are necessary */
	XMLElement *info = s->skinlayout.NewElement("Info");
	XMLElement *resource = s->skinlayout.NewElement("Resource");
	XMLElement *skin = s->skinlayout.NewElement("Skin");
	XMLElement *option = s->skinlayout.NewElement("Option");
	s->skinlayout.LinkEndChild(info);
	s->skinlayout.LinkEndChild(option);
	s->skinlayout.LinkEndChild(resource);
	s->skinlayout.LinkEndChild(skin);
	info->SetAttribute("width", 1280);
	info->SetAttribute("height", 760);

	// load skin line
	// because lr2skin file format has no end tag, 
	//  it's better to load all lines before parsing
	line_total = LoadSkin(filepath);

	// after we read all lines, skin parse start
	condition_element[0] = cur_e = skin;
	condition_level = 0;
	ParseSkin();

	if (condition_level == 0) {
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

	char line[MAX_LINE_CHARACTER];
	char *p;
	int current_line = 0;		// current file's reading line
	while (!feof(f)) {
		current_line++;
		if (!fgets(line, 1024, f))
			break;
		p = Trim(line);

		// ignore comment
		if (strncmp("//", p, 2) == 0 || !strlen(p))
			continue;

		// if #include then read the file first
		if (strncmp("#INCLUDE", p, 8) == 0) {
			char *np = strchr(p + 9, ',');
			if (np) *np = 0;
			std::string relpath = p + 9;
			std::string basepath = filepath;
			ConvertLR2PathToRelativePath(relpath);
			GetParentDirectory(basepath);
			ConvertRelativePathToAbsPath(relpath, basepath);
			linebufferpos += LoadSkin(relpath.c_str(), linebufferpos + 1);
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

/* a simple private macro for PLAYLANE (reset position) */
void MakeFrameRelative(int x, int y, XMLElement *frame) {
	frame->SetAttribute("x", frame->IntAttribute("x") - x);
	frame->SetAttribute("y", frame->IntAttribute("y") - y);
}
void MakeRelative(int x, int y, XMLElement *e) {
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		XMLElement *frame = dst->FirstChildElement("Frame");
		while (frame) {
			MakeFrameRelative(x, y, frame);
			frame = frame->NextSiblingElement("Frame");
		}
		dst = e->NextSiblingElement("DST");
	}
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
		// #ELSE: find last #IF clause, and copy condition totally.
		// not perfect, but maybe we can make a deal :)
		XMLElement *prev_if = condition_element[condition_level - 1]->LastChildElement("If");
		if (prev_if) {
			XMLElement *group = s->skinlayout.NewElement("Ifnot");
			group->SetAttribute("condition", prev_if->Attribute("condition"));
			cur_e = condition_element[--condition_level];
			cur_e->LinkEndChild(group);
			condition_element[++condition_level] = cur_e = group;
		}
		else {
			// we tend to ignore #ELSE clause ... it's wrong.
			cur_e = 0;
		}
		return line + 1;
	}
	else if (CMD_IS("#IF") || CMD_IS("#ELSEIF")) {
		// #ELSEIF: think as new #IF clause
		XMLElement *group = s->skinlayout.NewElement("If");
		if (CMD_IS("#IF")) condition_level++;
		condition_element[condition_level] = group;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = INT(args[i])?TranslateOPs(INT(args[i])):0;
			if (c) cls.AddCondition(c);
		}
		group->SetAttribute("condition", cls.ToString());
		cur_e = condition_element[condition_level - 1];		// get parent object
		cur_e->LinkEndChild(group);
		cur_e = group;
		return line + 1;
	}

	/*
	* header/metadata parsing
	*/
	if (CMD_IS("#CUSTOMOPTION")) {
		XMLElement *customoption = s->skinlayout.NewElement("CustomSwitch");
		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		customoption->SetAttribute("name", name_safe.c_str());
		int option_intvalue = atoi(args[2]);
		for (char **p = args + 3; *p != 0 && strlen(*p) > 0; p++) {
			XMLElement *options = s->skinlayout.NewElement("Option");
			options->SetAttribute("name", *p);
			options->SetAttribute("value", option_intvalue);
			option_intvalue++;
			customoption->LinkEndChild(options);
		}
		XMLElement *option = s->skinlayout.FirstChildElement("Option");
		option->LinkEndChild(customoption);
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		XMLElement *customfile = s->skinlayout.NewElement("CustomFile");
		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		customfile->SetAttribute("name", name_safe.c_str());
		// decide file type
		std::string path_safe = args[2];
		ReplaceString(path_safe, "\\", "/");
		if (FindString(path_safe.c_str(), "*.jpg") || FindString(path_safe.c_str(), "*.png"))
			customfile->SetAttribute("type", "file/image");
		else if (FindString(path_safe.c_str(), "*.mpg") || FindString(path_safe.c_str(), "*.mpeg") || FindString(path_safe.c_str(), "*.avi"))
			customfile->SetAttribute("type", "file/video");
		else if (FindString(path_safe.c_str(), "*.ttf"))
			customfile->SetAttribute("type", "file/ttf");
		else if (FindString(path_safe.c_str(), "/*"))
			customfile->SetAttribute("type", "folder");		// it's somewhat ambiguous...
		else
			customfile->SetAttribute("type", "file");
		std::string path = args[2];
		ReplaceString(path, "*", args[3]);
		ConvertLR2PathToRelativePath(path);
		customfile->SetAttribute("path", path.c_str());		// means default path
		XMLElement *option = s->skinlayout.FirstChildElement("Option");
		option->LinkEndChild(customfile);

		// register to filter_to_optionname for image change
		filter_to_optionname.insert(std::pair<std::string, std::string>(args[2], name_safe));
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
		case 5:
			// select screen
			type->SetText("SelectMusic");
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
		image->SetAttribute("name", image_cnt++);
		// check for optionname path
		std::string path_converted = args[1];
		for (auto it = filter_to_optionname.begin(); it != filter_to_optionname.end(); ++it) {
			if (FindString(args[1], it->first.c_str())) {
				// replace filtered path to reserved name
				ReplaceString(path_converted, it->first, "$(" + it->second + ")");
				break;
			}
		}
		// convert path to relative
		ReplaceString(path_converted, "\\", "/");
		ConvertLR2PathToRelativePath(path_converted);
		image->SetAttribute("path", path_converted.c_str());

		resource->LinkEndChild(image);
	}
	else if (CMD_IS("#FONT")) {
		// we don't use bitmap fonts
		// So if cannot found, we'll going to use default font/texture.
		// current font won't support TTF, so basically we're going to use default font.
		XMLElement *resource = s->skinlayout.FirstChildElement("Resource");
		XMLElement *font = s->skinlayout.NewElement("Font");
		font->SetAttribute("name", font_cnt++);
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
		XMLElement *setoption = s->skinlayout.NewElement("Lua");
		std::ostringstream luacode;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = INT(args[i]) ? TranslateOPs(INT(args[i])) : 0;
			if (c)
				luacode << "SetTimer(\"" << c << "\")\n";
		}
		setoption->SetText(("\n" + luacode.str()).c_str());
		cur_e->LinkEndChild(setoption);
	}
	/* Just ignore these option */
	else if (CMD_IS("#ENDOFHEADER")) {}
	else if (CMD_IS("#FLIPRESULT")) {}
	else if (CMD_IS("#TRANSCLOLR")) {}		// should we have to implement colorkey?
	else if (CMD_STARTSWITH("#SRC_", 4)){
		// we parse #DST with #SRC.
		// process SRC
		// SRC may have condition (attribute condition; normally not used)
		XMLElement *obj;
		obj = s->skinlayout.NewElement("Image");
		int resid = INT(args[2]);
		obj->SetAttribute("resid", resid);
		obj->SetAttribute("x", INT(args[3]));
		obj->SetAttribute("y", INT(args[4]));
		if (INT(args[5]) > 0) {
			obj->SetAttribute("w", INT(args[5]));
			obj->SetAttribute("h", INT(args[6]));
		}
		if (INT(args[7]) > 1 || INT(args[8]) > 1) {
			obj->SetAttribute("divx", INT(args[7]));
			obj->SetAttribute("divy", INT(args[8]));
		}
		if (INT(args[9]))
			obj->SetAttribute("cycle", INT(args[9]));
		int sop1 = 0, sop2 = 0, sop3 = 0;
		if (INT(args[10]))
			obj->SetAttribute("timer", TranslateTimer(INT(args[10])));
		if (args[11]) sop1 = INT(args[11]);
		if (args[12]) sop2 = INT(args[12]);
		if (args[13]) sop3 = INT(args[13]);

		/*
		 * process NOT-general-objects first
		 * these objects doesn't have #DST object directly
		 * (bad-syntax >:( )
		 */
		// check for play area
		int isPlayElement = ProcessLane(obj, line, resid);
		if (isPlayElement) {
			return isPlayElement;
		}

		/*
		 * under these are objects which requires #DST object directly
		 * (good syntax)
		 * we have to make object now to parse #DST
		 * and, if unable to figure out what this element is, it'll be considered as Image object.
		 */

		int looptime = 0, blend = 0, timer = 0, rotatecenter = -1, acc = 0;
		int op1 = 0, op2 = 0, op3 = 0;
		int nl;
		XMLElement *dst = s->skinlayout.NewElement("DST");
		obj->LinkEndChild(dst);
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
				acc = INT(args[7]);
				if (args[18]) op1 = INT(args[18]);
				if (args[19]) op2 = INT(args[19]);
				if (args[20]) op3 = INT(args[20]);
			}
			XMLElement *frame = s->skinlayout.NewElement("Frame");
			frame->SetAttribute("x", args[3]);
			frame->SetAttribute("y", args[4]);
			frame->SetAttribute("w", args[5]);
			frame->SetAttribute("h", args[6]);
			frame->SetAttribute("time", args[2]);
			if (!(INT(args[8]) == 255))
				frame->SetAttribute("a", args[8]);
			if (!(INT(args[9]) == 255 && INT(args[10]) == 255 && INT(args[11]) == 255)) {
				frame->SetAttribute("r", args[9]);
				frame->SetAttribute("g", args[10]);
				frame->SetAttribute("b", args[11]);
			}
			if (INT(args[14]))
				frame->SetAttribute("angle", args[14]);
			if (INT(args[2]) == looptime)
				frame->SetAttribute("loop", true);
			dst->LinkEndChild(frame);
		}
		// set common draw attribute
		dst->SetAttribute("acc", acc);
		if (blend > 1)
			dst->SetAttribute("blend", blend);
		if (rotatecenter > 0)
			dst->SetAttribute(
			"rotatecenter", rotatecenter);
		if (TranslateTimer(timer))
			dst->SetAttribute("timer", TranslateTimer(timer));
		ConditionAttribute cls;
		const char *c;
		c = op1?TranslateOPs(op1):0;
		if (c) cls.AddCondition(c);
		c = op2?TranslateOPs(op2):0;
		if (c) cls.AddCondition(c);
		c = op3?TranslateOPs(op3):0;
		if (c) cls.AddCondition(c);
		if (cls.GetConditionNumber())
			obj->SetAttribute("condition", cls.ToString());
		// before register, check loop statement (is loop is in last object, it isn't necessary)
		if (dst->LastChild() && dst->LastChild()->ToElement()->Attribute("loop")) {
			dst->LastChild()->ToElement()->DeleteAttribute("loop");
		}


		/*
		* If object is select screen panel dependent(timer/op code 2x),
		* then add object to there (ease of control)
		* TODO: take care of 3x objects (OnPanelClose)
		*/
#define CHECK_PANEL(v) (op1 == (v) || op2 == (v) || op3 == (v) || timer == (v))
		if (CHECK_PANEL(21))
			FindElementWithAttribute(cur_e, "Group", "id", "panel1", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(22))
			FindElementWithAttribute(cur_e, "Group", "id", "panel2", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(23))
			FindElementWithAttribute(cur_e, "Group", "id", "panel3", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(24))
			FindElementWithAttribute(cur_e, "Group", "id", "panel4", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(25))
			FindElementWithAttribute(cur_e, "Group", "id", "panel5", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(26))
			FindElementWithAttribute(cur_e, "Group", "id", "panel6", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(27))
			FindElementWithAttribute(cur_e, "Group", "id", "panel7", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(28))
			FindElementWithAttribute(cur_e, "Group", "id", "panel8", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(29))
			FindElementWithAttribute(cur_e, "Group", "id", "panel9", &s->skinlayout)->LinkEndChild(obj);
		else
			cur_e->LinkEndChild(obj);
		

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

		// combo (play)
		int isComboElement = ProcessCombo(obj, line);
		if (isComboElement)
			return isComboElement;

		// select menu (select)
		int isSelectBar = ProcessSelectBar(obj, line);
		if (isSelectBar)
			return isSelectBar;

		/* 
		 * under these are general individual object
		 */
		if (OBJTYPE_IS("IMAGE")) {
			// nothing to do (general object)
			// but it's not in some special OP/Timer code
			// - include BOMB/LANE effect into PLAYLANE object
			// (we may want to include BOMB/OnBeat, but it's programs's limit. Can't do it now.)
			// (do it yourself)
			if (!(obj->IntAttribute("w") < 100 && obj->IntAttribute("h") < 100)) {
				if (//(timer >= 50 && timer < 60) ||
					(timer >= 70 && timer < 80) ||
					(timer >= 100 && timer < 110) ||
					(timer >= 120 && timer < 130)) {
					// P1
					XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", 0, &s->skinlayout);
					MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
					playarea->LinkEndChild(obj);
				}
				else if (//(timer >= 60 && timer < 70) ||
					(timer >= 80 && timer < 90) ||
					(timer >= 110 && timer < 120) ||
					(timer >= 130 && timer < 140)) {
					// P2
					XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", 1, &s->skinlayout);
					MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
					playarea->LinkEndChild(obj);
				}
			}
			// however, BOMB SRC effect(SRC loop) MUST TURN OFF
			if ((timer >= 50 && timer < 60) || (timer >= 60 && timer < 70))
				obj->SetAttribute("loop", 0);
		}
		else if (OBJTYPE_IS("BGA")) {
			// change tag to BGA and remove SRC tag
			obj->SetName("Bga");
		}
		else if (OBJTYPE_IS("NUMBER")) {
			obj->SetName("Number");
			// just convert SRC to texturefont ...
			ConvertToTextureFont(obj);
			/*
			* Number object will act just like a extended-string object.
			* If no value, then it'll just show '0' value.
			*/
			if (TranslateNumber(sop1))
				obj->SetAttribute("value", TranslateNumber(sop1));
			obj->SetAttribute("align", sop2);
			/*
			* if type == 11 or 24, then set length
			* if type == 24, then set '24mode' (only for LR2 - depreciated supportance)
			* (If you want to implement LR2-like font, then you may have to make 2 type of texturefont SRC -
			* plus and minus - with proper condition.)
			*/
			if (sop3)
				dst->SetAttribute("length", sop3);
		}
		else if (OBJTYPE_IS("SLIDER")) {
			// change tag to slider and add attr
			obj->SetName("Slider");
			obj->SetAttribute("direction", sop1);
			obj->SetAttribute("range", sop2);
			if (TranslateSlider(sop3))
				obj->SetAttribute("value", TranslateSlider(sop3));
			//obj->SetAttribute("range", sop2); - disable option is ignored
		}
		else if (OBJTYPE_IS("TEXT")) {
			// delete src attr and change to font/st/align/edit
			obj->DeleteAttribute("x");
			obj->DeleteAttribute("y");
			obj->DeleteAttribute("w");
			obj->DeleteAttribute("h");
			obj->DeleteAttribute("cycle");
			obj->DeleteAttribute("divx");
			obj->DeleteAttribute("divy");
			obj->SetName("Text");
			if (TranslateText(INT(args[3])))
				obj->SetAttribute("value", TranslateText(INT(args[3])));
			obj->SetAttribute("align", args[4]);
			obj->SetAttribute("edit", args[5]);
		}
		else if (OBJTYPE_IS("BARGRAPH")) {
			obj->SetName("Graph");
			if (TranslateGraph(sop1))
				obj->SetAttribute("value", TranslateGraph(sop1));
			obj->SetAttribute("direction", sop2);
		}
		else if (OBJTYPE_IS("BUTTON")) {
			// TODO: onclick event
			obj->SetName("Button");
		}
		/* 
		 * some special object (PLAY lane object) 
		 */
		else if (OBJTYPE_IS("JUDGELINE")) {
			obj->SetName("JUDGELINE");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("LINE")) {
			obj->SetName("LINE");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("ONMOUSE")) {
			// depreciated/ignore
			// TODO: support this by SRC_HOVER tag.
			printf("#XXX_ONMOUSE command is depreciated, ignore. (%dL) \n", line);
		}
		else {
			printf("Unknown General Object (%s), consider as IMAGE. (%dL)\n", args[0] + 5, line_position[line]);
		}

		// return new line
		return nl;
	}
	/*
	 * SELECT part 
	 */
	else if (CMD_STARTSWITH("#DST_BAR_BODY", 13)) {
		// select menu part
		int isProcessSelectBarDST = ProcessSelectBar_DST(line);
		if (isProcessSelectBarDST)
			return isProcessSelectBarDST;
	}
	else if (CMD_IS("#BAR_CENTER")) {
		// set center and property ...
		XMLElement *selectmenu = FindElement(cur_e, "SelectMenu");
		selectmenu->SetAttribute("center", line_args[line][1]);
	}
	else if (CMD_IS("#BAR_AVAILABLE")) {
		// depreciated, not parse
		printf("#BAR_AVAILABLE - depreciated, Ignore.\n");
	}
	/*
	 * PLAY part 
	 */
	else if (CMD_STARTSWITH("#DST_NOTE", 9)) {
		char **args = line_args[line];
		int objectid = INT(args[1]);
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		lane->SetAttribute("x", INT(args[3]) - playarea->IntAttribute("x"));
		lane->SetAttribute("y", INT(args[4]) - playarea->IntAttribute("y"));
		lane->SetAttribute("w", INT(args[5]));
		lane->SetAttribute("h", INT(args[6]));
	}
	/*
	 * etc
	 */
	else if (CMD_STARTSWITH("#DST_", 5)) {
		// just ignore
	}
	else {
		printf("Unknown Type: %s (%dL) - Ignore.\n", args[0], line_position[line]);
	}

	// parse next line
	return line + 1;
}

/*
 * process commands about lane
 * if not lane, return 0
 * if lane, return next parsed line
 */
int _LR2SkinParser::ProcessLane(XMLElement *src, int line, int resid) {
	char **args = line_args[line];
	int objectid = INT(args[1]);

	if (OBJTYPE_IS("NOTE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		lane->SetAttribute("resid", resid);
		// add src to here
		src->SetName("SRC_NOTE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_END")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_END");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_BODY")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_BODY");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("LN_START")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_LN_START");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("MINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_MINE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	if (OBJTYPE_IS("AUTO_NOTE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_NOTE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_END")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_END");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_BODY")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_BODY");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_LN_START")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_LN_START");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("AUTO_MINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "Note", "index", objectid, &s->skinlayout);
		// add src to here
		src->SetName("SRC_AUTO_MINE");
		lane->LinkEndChild(src);
		return line + 1;
	}
	else if (OBJTYPE_IS("JUDGELINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "Play", "side", objectid, &s->skinlayout);
		// find DST object to set Lane attribute
		for (int _l = line + 1; _l < line_total; _l++) {
			if (line_args[_l][0] && strcmp(line_args[_l][0], "#DST_JUDGELINE") == 0 && INT(line_args[_l][1]) == objectid) {
				int x = INT(line_args[_l][3]);
				int y = 0;
				int w = INT(line_args[_l][5]);
				int h = INT(line_args[_l][4]);
				playarea->SetAttribute("x", x);
				playarea->SetAttribute("y", y);
				playarea->SetAttribute("w", w);
				playarea->SetAttribute("h", h);
				XMLElement *dst = FindElement(playarea, "DST", &s->skinlayout);
				XMLElement *frame = FindElement(dst, "Frame", &s->skinlayout);
				frame->SetAttribute("x", x);
				frame->SetAttribute("y", y);
				frame->SetAttribute("w", w);
				frame->SetAttribute("h", h);
				break;
			}
		}
		// but don't add SRC; we'll add it to PlayArea
		return 0;
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

int _LR2SkinParser::ProcessSelectBar(XMLElement *obj, int line) {
	char **args = line_args[line];
	int objectid = INT(args[1]);

	// select menu part
	if (!OBJTYPE_IS("BARGRAPH") && CMD_STARTSWITH("#SRC_BAR", 8)) {
		XMLElement *selectmenu = FindElement(cur_e, "SelectMenu", &s->skinlayout);
		if (OBJTYPE_IS("BAR_BODY")) {
			// only register SRC object
			XMLElement *src = obj->FirstChildElement("SRC");
			src->SetName("SRC_BODY");
			src->SetAttribute("type", objectid);	// foldertype
			selectmenu->LinkEndChild(src);
			// should remove parent object
			obj->DeleteChildren();
			s->skinlayout.DeleteNode(obj);
		}
		else if (OBJTYPE_IS("BAR_FLASH")) {
			obj->SetName("Flash");
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_TITLE")) {
			obj->SetName("Title");
			obj->SetAttribute("type", objectid);	// new: 1
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LEVEL")) {
			obj->SetName("Level");
			obj->SetAttribute("type", objectid);	// difficulty
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LAMP")) {
			obj->SetName("Lamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_MY_LAMP")) {
			obj->SetName("MyLamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_RIVAL_LAMP")) {
			obj->SetName("RivalLamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_RIVAL")) {
			// ignore
			s->skinlayout.DeleteNode(obj);
		}
		else if (OBJTYPE_IS("BAR_RANK")) {
			// ignore
			s->skinlayout.DeleteNode(obj);
		}
		return line + 1;
	}

	// not a select bar object
	return 0;
}

int _LR2SkinParser::ProcessSelectBar_DST(int line) {
	char **args = line_args[line];
	int objectid = INT(args[1]);
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
	if (CMD_IS("#DST_BAR_BODY_ON")) {
		XMLElement *selectmenu = FindElement(cur_e, "SelectMenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "Position", &s->skinlayout);
		XMLElement *bodyoff = FindElement(position, "Bar");
		if (bodyoff) {
			/*
			* DST_SELECTED: only have delta_x, delta_y value
			*/
			XMLElement *frame = FindElement(bodyoff, "Frame");
			position->SetAttribute("delta_x", INT(args[3]) - frame->IntAttribute("x"));
		}
		return line + 1;
	}
	else if (CMD_IS("#DST_BAR_BODY_OFF")) {
		XMLElement *selectmenu = FindElement(cur_e, "SelectMenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "Position", &s->skinlayout);
		XMLElement *bodyoff = FindElementWithAttribute(position, "Bar", "index", INT(args[1]), &s->skinlayout);
		XMLElement *frame = s->skinlayout.NewElement("Frame");
		frame->SetAttribute("time", INT(args[2]));
		frame->SetAttribute("x", INT(args[3]));
		frame->SetAttribute("y", INT(args[4]));
		frame->SetAttribute("w", INT(args[5]));
		frame->SetAttribute("h", INT(args[6]));
		bodyoff->LinkEndChild(frame);
		return line + 1;
	}
	else {
		// not a select bar object
		return 0;
	}
}

void _LR2SkinParser::ConvertToTextureFont(XMLElement *obj) {
	XMLElement *dst = obj->FirstChildElement("DST");

	/*
	* Number uses Texturefont
	* (Number object itself is quite depreciated, so we'll going to convert it)
	* - Each number creates one texturefont
	* - texturefont syntax: *.ini file, basically.
	*   so, we have to convert Shift_JIS code into UTF-8 code.
	*   (this would be a difficult job, hmm.)
	*
	* if type == 10, then it's a just normal texture font
	* if type == 11,
	* if type == 22, convert like 11 (+ add new SRC for negative value)
	* if SRC is timer-dependent, then make multiple fonts and add condition
	* (maybe somewhat sophisticated condition)
	*/
	int glyphcnt = obj->IntAttribute("divx") * obj->IntAttribute("divy");
	int fonttype = 10;
	if (glyphcnt % 11 == 0)
		fonttype = 11;	// '*' glyph for empty space
	else if (glyphcnt % 24 == 0)
		fonttype = 24;	// +/- font included (so, 2 fonts will be created)

	/*
	 * Create font  set it
	 */
	int tfont_idx;
	tfont_idx = GenerateTexturefontString(obj);
	obj->SetAttribute("resid", tfont_idx);
	/*
	 * Processed little differently if it's 24mode.
	 * only for LR2, LEGACY attribute.
	 * negative number will be matched with Alphabet.
	 * Don't use this for general purpose, negative number will drawn incorrect.
	 * if you want, use Lua condition. that'll be helpful.
	 */
	if (fonttype == 24)
		obj->SetAttribute("24mode", true);
}

// returns new(or previous) number
int _LR2SkinParser::GenerateTexturefontString(XMLElement *obj) {
	char _temp[1024];
	std::string out;
	// get all attributes first
	int x = obj->IntAttribute("x");
	int y = obj->IntAttribute("y");
	int w = obj->IntAttribute("w");
	int h = obj->IntAttribute("h");
	int divx = obj->IntAttribute("divx");
	int divy = obj->IntAttribute("divy");
	int dw = w / divx;
	int dh = h / divy;
	int glyphcnt = obj->IntAttribute("divx") * obj->IntAttribute("divy");
	int fonttype = 10;
	if (glyphcnt % 11 == 0)
		fonttype = 11;
	else if (glyphcnt % 24 == 0)
		fonttype = 24;
	int repcnt = glyphcnt / fonttype;

	// oh we're too tired to make every new number
	// so, we're going to reuse previous number if it exists
	int id = w | h << 12 | obj->IntAttribute("name") << 24;	//	kind of id
	if (texturefont_id.find(id) != texturefont_id.end()) {
		return texturefont_id[id];
	}
	texturefont_id.insert(std::pair<int, int>(id, font_cnt++));

	// get image file path from resource
	XMLElement *resource = s->skinlayout.FirstChildElement("Resource");
	XMLElement *img = FindElementWithAttribute(resource, "Image", "name", obj->Attribute("name"));
	// create font data
	SkinTextureFont tfont;
	tfont.AddImageSrc(img->Attribute("path"));
	tfont.SetCycle(obj->IntAttribute("cycle"));
	char glyphs[] = "0123456789*+ABCDEFGHIJ#-";
	for (int r = 0; r < repcnt; r++) {
		for (int i = 0; i < fonttype; i++) {
			int cx = i % divx;
			int cy = i / divx;
			tfont.AddGlyph(glyphs[i], 0, x + dw * cx, y + dh * cy, dw, dh);
		}
	}
	tfont.SaveToText(out);
	out = "\n# Auto-generated texture font data by SkinParser\n" + out;

	// register to LR2 resource
	XMLElement *res = s->skinlayout.FirstChildElement("Resource");
	XMLElement *restfont = s->skinlayout.NewElement("TextureFont");
	restfont->SetAttribute("name", font_cnt-1);		// create new texture font
	restfont->SetAttribute("type", "1");			// for LR2 font type. (but not decided for other format, yet.)
	restfont->SetText(out.c_str());
	res->LinkEndChild(restfont);

	return font_cnt-1;
}

#define SETOPTION(s) (strcat(translated, s))
#define SETNEGATIVEOPTION(s) \
if (translated[0] == '!') translated[0] = 0;\
else strcpy(translated, "!");\
strcat(translated, s);
const char* _LR2SkinParser::TranslateOPs(int op) {
	/*
	 * In Rhythmus, there's no object called OP(condition) code. but, all conditions have timer code, and that does OP code's work.
	 * So this function will translate OP code into a valid condition code.
	 */
	if (op < 0) {
		strcpy(translated, "!");
		op *= -1;
	}
	else {
		translated[0] = 0;
	}

	if (op == 0) {
		/* this is an constant code - but should NOT given as argument, I suggest. */
		strcat(translated, "true");
	}
	else if (op > 0 && op < 300) {
		if (op == 1) {
			strcat(translated, "IsSelectBarFolder");
		}
		else if (op == 2) {
			strcat(translated, "IsSelectBarSong");
		}
		else if (op == 3) {
			strcat(translated, "IsSelectBarCourse");
		}
		else if (op == 4) {
			strcat(translated, "IsSelectBarNewCourse");
		}
		else if (op == 5) {
			strcat(translated, "IsSelectBarPlayable");
		}
		else if (op == 10) {
			strcat(translated, "IsDoublePlay");
		}
		else if (op == 11) {
			strcat(translated, "IsBattlePlay");
		}
		else if (op == 12) {
			strcat(translated, "IsDoublePlay");	// this includes battle
		}
		else if (op == 13) {
			strcat(translated, "IsBattlePlay");	// this includes ghost battle
		}
		else if (op == 20) {
			strcat(translated, "OnPanel");
		}
		else if (op == 21) {
			strcat(translated, "OnPanel1");
		}
		else if (op == 22) {
			strcat(translated, "OnPanel2");
		}
		else if (op == 23) {
			strcat(translated, "OnPanel3");
		}
		else if (op == 24) {
			strcat(translated, "OnPanel4");
		}
		else if (op == 25) {
			strcat(translated, "OnPanel5");
		}
		else if (op == 26) {
			strcat(translated, "OnPanel6");
		}
		else if (op == 27) {
			strcat(translated, "OnPanel7");
		}
		else if (op == 28) {
			strcat(translated, "OnPanel8");
		}
		else if (op == 29) {
			strcat(translated, "OnPanel9");
		}
		else if (op == 30) {
			strcat(translated, "IsBGANormal");		// Depreciated; won't be used
		}
		else if (op == 31) {
			strcat(translated, "IsBGA");
		}
		else if (op == 32) {
			SETNEGATIVEOPTION("IsAutoPlay");
		}
		else if (op == 33) {
			strcat(translated, "IsAutoPlay");
		}
		else if (op == 34) {
			strcat(translated, "IsGhostOff");			// hmm ...
		}
		else if (op == 35) {
			strcat(translated, "IsGhostA");
		}
		else if (op == 36) {
			strcat(translated, "IsGhostB");
		}
		else if (op == 37) {
			strcat(translated, "IsGhostC");
		}
		else if (op == 38) {
			SETNEGATIVEOPTION("IsScoreGraph");
		}
		else if (op == 39) {
			SETOPTION("IsScoreGraph");
		}
		else if (op == 40) {
			SETNEGATIVEOPTION("IsBGA");
		}
		else if (op == 41) {
			strcat(translated, "IsBGA");
		}
		else if (op == 42) {
			strcat(translated, "Is1PGrooveGuage");
		}
		else if (op == 43) {
			strcat(translated, "Is1PHardGuage");
		}
		else if (op == 44) {
			strcat(translated, "Is2PGrooveGuage");
		}
		else if (op == 45) {
			strcat(translated, "Is2PHardGuage");
		}
		else if (op == 46) {
			strcat(translated, "IsDiffFiltered");		// on select menu; but depreciated?
		}
		else if (op == 47) {
			SETNEGATIVEOPTION("IsDifficultyFilter");
		}
		else if (op == 50) {
			SETNEGATIVEOPTION("IsOnline");
		}
		else if (op == 51) {
			strcat(translated, "IsOnline");
		}
		else if (op == 52) {
			SETNEGATIVEOPTION("IsExtraMode");			// DEMO PLAY
		}
		else if (op == 53) {
			strcat(translated, "IsExtraMode");
		}
		else if (op == 54) {
			SETNEGATIVEOPTION("Is1PAutoSC");
		}
		else if (op == 55) {
			strcat(translated, "Is1PAutoSC");
		}
		else if (op == 56) {
			if (translated[0] == '!') translated[0] = 0;
			else strcpy(translated, "!");
			strcat(translated, "Is2PAutoSC");
		}
		else if (op == 57) {
			strcat(translated, "Is2PAutoSC");
		}
		else if (op == 60) {
			if (translated[0] == '!') translated[0] = 0;
			else strcpy(translated, "!");
			strcat(translated, "IsRecordable");
		}
		else if (op == 61) {
			strcat(translated, "IsRecordable");
		}
		else if (op == 62) {
			if (translated[0] == '!') translated[0] = 0;
			else strcpy(translated, "!");
			strcat(translated, "IsRecordable");
		}
		else if (op == 63) {
			strcat(translated, "IsEasyClear");
		}
		else if (op == 64) {
			strcat(translated, "IsGrooveClear");
		}
		else if (op == 65) {
			strcat(translated, "IsHardClear");
		}/* NO EXH in LR2
		else if (op == 66) {
		strcat(translated, "IsEXHClear");
		}*/
		else if (op == 66) {
			strcat(translated, "IsFCClear");
		}
		else if (op == 70) {
			SETNEGATIVEOPTION("IsBeginnerSparkle");
		}
		else if (op == 71) {
			SETNEGATIVEOPTION("IsNormalSparkle");
		}
		else if (op == 72) {
			SETNEGATIVEOPTION("IsHyperSparkle");
		}
		else if (op == 73) {
			SETNEGATIVEOPTION("IsAnotherSparkle");
		}
		else if (op == 74) {
			SETNEGATIVEOPTION("IsInsaneSparkle");
		}
		else if (op == 75) {
			strcat(translated, "IsBeginnerSparkle");
		}
		else if (op == 76) {
			strcat(translated, "IsNormalSparkle");
		}
		else if (op == 77) {
			strcat(translated, "IsHyperSparkle");
		}
		else if (op == 78) {
			strcat(translated, "IsAnotherSparkle");
		}
		else if (op == 79) {
			strcat(translated, "IsInsaneSparkle");
		}
		else if (op == 80) {
			strcat(translated, "OnSongLoading");
		}
		else if (op == 81) {
			strcat(translated, "OnSongLoadingEnd");
		}
		else if (op == 84) {
			strcat(translated, "OnSongReplay");
		}
		else if (op == 90) {
			strcat(translated, "OnResultClear");
		}
		else if (op == 91) {
			strcat(translated, "OnResultFail");
		}
		else if (op == 150) {
			strcat(translated, "OnDiffNone");			// I suggest to use DiffValue == 0 then this.
		}
		else if (op == 151) {
			strcat(translated, "OnDiffBeginner");
		}
		else if (op == 152) {
			strcat(translated, "OnDiffNormal");
		}
		else if (op == 153) {
			strcat(translated, "OnDiffHyper");
		}
		else if (op == 154) {
			strcat(translated, "OnDiffAnother");
		}
		else if (op == 155) {
			strcat(translated, "OnDiffInsane");
		}
		else if (op == 160) {
			strcat(translated, "Is7Keys");
		}
		else if (op == 161) {
			strcat(translated, "Is5Keys");
		}
		else if (op == 162) {
			strcat(translated, "Is14Keys");
		}
		else if (op == 163) {
			strcat(translated, "Is10Keys");
		}
		else if (op == 164) {
			strcat(translated, "Is9Keys");
		}
		/* 165 ~ : should we work with it? (same with op 160 ~ 161) */
		else if (op == 170) {
			SETOPTION("IsBGA");
		}
		else if (op == 171) {
			SETNEGATIVEOPTION("IsBGA");
		}
		else if (op == 172) {
			SETOPTION("IsLongNote");
		}
		else if (op == 173) {
			SETNEGATIVEOPTION("IsLongNote");
		}
		else if (op == 174) {
			SETOPTION("IsBmsReadme");
		}
		else if (op == 175) {
			SETNEGATIVEOPTION("IsBmsReadme");
		}
		else if (op == 176) {
			SETOPTION("IsBpmChange");
		}
		else if (op == 177) {
			SETNEGATIVEOPTION("IsBpmChange");
		}
		else if (op == 178) {
			SETOPTION("IsBmsRandomCommand");
		}
		else if (op == 179) {
			SETNEGATIVEOPTION("IsBmsRandomCommand");
		}
		else if (op == 180) {
			SETOPTION("IsJudgeVERYHARD");
		}
		else if (op == 181) {
			SETOPTION("IsJudgeHARD");
		}
		else if (op == 182) {
			SETOPTION("IsJudgeNORMAL");
		}
		else if (op == 183) {
			SETOPTION("IsJudgeEASY");
		}
		else if (op == 184) {
			SETOPTION("IsLevelSparkle");
		}
		else if (op == 185) {
			SETNEGATIVEOPTION("IsLevelSparkle");
		}
		else if (op == 186) {
			SETOPTION("IsLevelSparkle");
		}
		else if (op == 190) {
			SETNEGATIVEOPTION("IsStageFile");
		}
		else if (op == 191) {
			SETOPTION("IsStageFile");
		}
		else if (op == 192) {
			SETNEGATIVEOPTION("IsBANNER");
		}
		else if (op == 193) {
			SETOPTION("IsBANNER");
		}
		else if (op == 194) {
			SETNEGATIVEOPTION("IsBACKBMP");
		}
		else if (op == 195) {
			SETOPTION("IsBACKBMP");
		}
		else if (op == 196) {
			SETNEGATIVEOPTION("IsPlayable");
		}
		else if (op == 197) {
			SETOPTION("IsReplayable");
		}
	}
	/* during play */
	else if (op >= 200 && op < 300) {
		if (op == 200) {
			SETOPTION("Is1PAAA");
		}
		else if (op == 201) {
			SETOPTION("Is1PAA");
		}
		else if (op == 202) {
			SETOPTION("Is1PA");
		}
		else if (op == 203) {
			SETOPTION("Is1PB");
		}
		else if (op == 204) {
			SETOPTION("Is1PC");
		}
		else if (op == 205) {
			SETOPTION("Is1PD");
		}
		else if (op == 206) {
			SETOPTION("Is1PE");
		}
		else if (op == 207) {
			SETOPTION("Is1PF");
		}
		else if (op == 210) {
			SETOPTION("Is2PAAA");
		}
		else if (op == 211) {
			SETOPTION("Is2PAA");
		}
		else if (op == 212) {
			SETOPTION("Is2PA");
		}
		else if (op == 213) {
			SETOPTION("Is2PB");
		}
		else if (op == 214) {
			SETOPTION("Is2PC");
		}
		else if (op == 215) {
			SETOPTION("Is2PD");
		}
		else if (op == 216) {
			SETOPTION("Is2PE");
		}
		else if (op == 217) {
			SETOPTION("Is2PF");
		}
		else if (op == 220) {
			SETOPTION("Is1PReachAAA");
		}
		else if (op == 221) {
			SETOPTION("Is1PReachAA");
		}
		else if (op == 222) {
			SETOPTION("Is1PReachA");
		}
		else if (op == 223) {
			SETOPTION("Is1PReachB");
		}
		else if (op == 224) {
			SETOPTION("Is1PReachC");
		}
		else if (op == 225) {
			SETOPTION("Is1PReachD");
		}
		else if (op == 226) {
			SETOPTION("Is1PReachE");
		}
		else if (op == 227) {
			SETOPTION("Is1PReachF");
		}
		/* 23X : I dont want to implement these useless one ... hmm... don't want ... .... */
		else if (op == 241) {
			SETOPTION("On1PJudgePerfect");
		}
		else if (op == 242) {
			SETOPTION("On1PJudgeGreat");
		}
		else if (op == 243) {
			SETOPTION("On1PJudgeGood");
		}
		else if (op == 244) {
			SETOPTION("On1PJudgeBad");
		}
		else if (op == 245) {
			SETOPTION("On1PJudgePoor");
		}
		else if (op == 246) {
			SETOPTION("On1PJudgePoor");		// ÍöPOOR
		}
		else if (op == 247) {
			SETOPTION("On1PPoorBGA");
		}
		else if (op == 248) {
			SETNEGATIVEOPTION("On1PPoorBGA");
		}/*
		else if (op == 220) {
		SETOPTION("Is1PReachAAA");
		}
		else if (op == 221) {
		SETOPTION("Is1PReachAA");
		}
		else if (op == 222) {
		SETOPTION("Is1PReachA");
		}
		else if (op == 223) {
		SETOPTION("Is1PReachB");
		}
		else if (op == 224) {
		SETOPTION("Is1PReachC");
		}
		else if (op == 225) {
		SETOPTION("Is1PReachD");
		}
		else if (op == 226) {
		SETOPTION("Is1PReachE");
		}
		else if (op == 227) {
		SETOPTION("Is1PReachF");
		}*/
		/* 23X : I dont want to implement these useless one ... hmm... don't want ... .... */
		else if (op == 261) {
			SETOPTION("On2PJudgePerfect");
		}
		else if (op == 262) {
			SETOPTION("On2PJudgeGreat");
		}
		else if (op == 263) {
			SETOPTION("On2PJudgeGood");
		}
		else if (op == 264) {
			SETOPTION("On2PJudgeBad");
		}
		else if (op == 265) {
			SETOPTION("On2PJudgePoor");
		}
		else if (op == 266) {
			SETOPTION("On2PJudgePoor");		// ÍöPOOR
		}
		else if (op == 267) {
			SETOPTION("On2PPoorBGA");
		}
		else if (op == 268) {
			SETNEGATIVEOPTION("On2PPoorBGA");
		}
		/* SUD/LIFT */
		else if (op == 270) {
			strcat(translated, "On1PSuddenChange");
		}
		else if (op == 271) {
			strcat(translated, "On2PSuddenChange");
		}
		/* Course related */
		else if (op == 280) {
			strcat(translated, "IsCourse1Stage");
		}
		else if (op == 281) {
			strcat(translated, "IsCourse2Stage");
		}
		else if (op == 282) {
			strcat(translated, "IsCourse3Stage");
		}
		else if (op == 283) {
			strcat(translated, "IsCourse4Stage");
		}
		else if (op == 289) {
			strcat(translated, "IsCourseFinal");
		}
		else if (op == 290) {
			strcat(translated, "IsCourse");
		}
		else if (op == 291) {
			strcat(translated, "IsGrading");		// Ó«êÈìãïÒ
		}
		else if (op == 292) {
			strcat(translated, "ExpertCourse");
		}
		else if (op == 293) {
			strcat(translated, "ClassCourse");
		}
	}
	/* result screen */
	else if (op >= 300 && op < 400) {
		if (op == 300) {
			SETOPTION("Is1PAAA");
		}
		else if (op == 301) {
			SETOPTION("Is1PAA");
		}
		else if (op == 302) {
			SETOPTION("Is1PA");
		}
		else if (op == 303) {
			SETOPTION("Is1PB");
		}
		else if (op == 304) {
			SETOPTION("Is1PC");
		}
		else if (op == 305) {
			SETOPTION("Is1PD");
		}
		else if (op == 306) {
			SETOPTION("Is1PE");
		}
		else if (op == 307) {
			SETOPTION("Is1PF");
		}
		else if (op == 310) {
			SETOPTION("Is2PAAA");
		}
		else if (op == 311) {
			SETOPTION("Is2PAA");
		}
		else if (op == 312) {
			SETOPTION("Is2PA");
		}
		else if (op == 313) {
			SETOPTION("Is2PB");
		}
		else if (op == 314) {
			SETOPTION("Is2PC");
		}
		else if (op == 315) {
			SETOPTION("Is2PD");
		}
		else if (op == 316) {
			SETOPTION("Is2PE");
		}
		else if (op == 317) {
			SETOPTION("Is2PF");
		}
		else if (op == 320) {
			SETOPTION("IsBeforeAAA");
		}
		else if (op == 321) {
			SETOPTION("IsBeforeAA");
		}
		else if (op == 322) {
			SETOPTION("IsBeforeA");
		}
		else if (op == 323) {
			SETOPTION("IsBeforeB");
		}
		else if (op == 324) {
			SETOPTION("IsBeforeC");
		}
		else if (op == 325) {
			SETOPTION("IsBeforeD");
		}
		else if (op == 326) {
			SETOPTION("IsBeforeE");
		}
		else if (op == 327) {
			SETOPTION("IsBeforeF");
		}
		else if (op == 340) {
			SETOPTION("IsAfterAAA");
		}
		else if (op == 341) {
			SETOPTION("IsAfterAA");
		}
		else if (op == 342) {
			SETOPTION("IsAfterA");
		}
		else if (op == 343) {
			SETOPTION("IsAfterB");
		}
		else if (op == 344) {
			SETOPTION("IsAfterC");
		}
		else if (op == 345) {
			SETOPTION("IsAfterD");
		}
		else if (op == 346) {
			SETOPTION("IsAfterE");
		}
		else if (op == 347) {
			SETOPTION("IsAfterF");
		}
		else if (op == 330) {
			SETOPTION("IsResultUpdated");
		}
		else if (op == 331) {
			SETOPTION("IsMaxcomboUpdated");
		}
		else if (op == 332) {
			SETOPTION("IsMinBPUpdated");
		}
		else if (op == 333) {
			SETOPTION("IsResultUpdated");	// ??
		}
		else if (op == 334) {
			SETOPTION("IsIRRankUpdated");
		}
		else if (op == 334) {
			SETOPTION("IsIRRankUpdated");	// ??
		}
	}
	/* TODO: change it into Lua script ...? */
	else if (op == 400) {
		SETOPTION("Is714KEY");
	}
	else if (op == 401) {
		SETOPTION("Is9KEY");
	}
	else if (op == 402) {
		SETOPTION("Is510KEY");
	}
	else if (op >= 900)
	{
		// #CUSTOMOPTION code
		itoa(op, translated, 10);
	}
	else {
		// unknown!
		// we can return '0', but I think it's better to return "UNKNOWN", for safety.
		return 0;
		//return "UNKNOWN";
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
	else if (timer == 21) {
		strcpy(translated, "OnPanel1");
	}
	else if (timer == 22) {
		strcpy(translated, "OnPanel2");
	}
	else if (timer == 23) {
		strcpy(translated, "OnPanel3");
	}
	else if (timer == 24) {
		strcpy(translated, "OnPanel4");
	}
	else if (timer == 25) {
		strcpy(translated, "OnPanel5");
	}
	else if (timer == 26) {
		strcpy(translated, "OnPanel6");
	}
	else if (timer == 27) {
		strcpy(translated, "OnPanel7");
	}
	else if (timer == 28) {
		strcpy(translated, "OnPanel8");
	}
	else if (timer == 29) {
		strcpy(translated, "OnPanel9");
	}
	/* Panel closing: DEPRECIATED */
	else if (timer == 31) {
		strcpy(translated, "OnPanel1Close");
	}
	else if (timer == 32) {
		strcpy(translated, "OnPanel2Close");
	}
	else if (timer == 33) {
		strcpy(translated, "OnPanel3Close");
	}
	else if (timer == 34) {
		strcpy(translated, "OnPanel4Close");
	}
	else if (timer == 35) {
		strcpy(translated, "OnPanel5Close");
	}
	else if (timer == 36) {
		strcpy(translated, "OnPanel6Close");
	}
	else if (timer == 37) {
		strcpy(translated, "OnPanel7Close");
	}
	else if (timer == 38) {
		strcpy(translated, "OnPanel8Close");
	}
	else if (timer == 39) {
		strcpy(translated, "OnPanel9Close");
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
		strcpy(translated, "On1PJudgeSCOkay");
	}
	else if (timer == 51) {
		strcpy(translated, "On1PJudge1Okay");
	}
	else if (timer == 52) {
		strcpy(translated, "On1PJudge2Okay");
	}
	else if (timer == 53) {
		strcpy(translated, "On1PJudge3Okay");
	}
	else if (timer == 54) {
		strcpy(translated, "On1PJudge4Okay");
	}
	else if (timer == 55) {
		strcpy(translated, "On1PJudge5Okay");
	}
	else if (timer == 56) {
		strcpy(translated, "On1PJudge6Okay");
	}
	else if (timer == 57) {
		strcpy(translated, "On1PJudge7Okay");
	}
	else if (timer == 58) {
		strcpy(translated, "On1PJudge8Okay");
	}
	else if (timer == 59) {
		strcpy(translated, "On1PJudge9Okay");
	}
	else if (timer == 60) {
		strcpy(translated, "On2PJudgeSCOkay");
	}
	else if (timer == 61) {
		strcpy(translated, "On2PJudge1Okay");
	}
	else if (timer == 62) {
		strcpy(translated, "On2PJudge2Okay");
	}
	else if (timer == 63) {
		strcpy(translated, "On2PJudge3Okay");
	}
	else if (timer == 64) {
		strcpy(translated, "On2PJudge4Okay");
	}
	else if (timer == 65) {
		strcpy(translated, "On2PJudge5Okay");
	}
	else if (timer == 66) {
		strcpy(translated, "On2PJudge6Okay");
	}
	else if (timer == 67) {
		strcpy(translated, "On2PJudge7Okay");
	}
	else if (timer == 68) {
		strcpy(translated, "On2PJudge8Okay");
	}
	else if (timer == 69) {
		strcpy(translated, "On2PJudge9Okay");
	}
	else if (timer == 70) {
		strcpy(translated, "On1PJudgeSCHold");
	}
	else if (timer == 71) {
		strcpy(translated, "On1PJudge1Hold");
	}
	else if (timer == 72) {
		strcpy(translated, "On1PJudge2Hold");
	}
	else if (timer == 73) {
		strcpy(translated, "On1PJudge3Hold");
	}
	else if (timer == 74) {
		strcpy(translated, "On1PJudge4Hold");
	}
	else if (timer == 75) {
		strcpy(translated, "On1PJudge5Hold");
	}
	else if (timer == 76) {
		strcpy(translated, "On1PJudge6Hold");
	}
	else if (timer == 77) {
		strcpy(translated, "On1PJudge7Hold");
	}
	else if (timer == 78) {
		strcpy(translated, "On1PJudge8Hold");
	}
	else if (timer == 79) {
		strcpy(translated, "On1PJudge9Hold");
	}
	else if (timer == 80) {
		strcpy(translated, "On2PJudgeSCHold");
	}
	else if (timer == 81) {
		strcpy(translated, "On2PJudge1Hold");
	}
	else if (timer == 82) {
		strcpy(translated, "On2PJudge2Hold");
	}
	else if (timer == 83) {
		strcpy(translated, "On2PJudge3Hold");
	}
	else if (timer == 84) {
		strcpy(translated, "On2PJudge4Hold");
	}
	else if (timer == 85) {
		strcpy(translated, "On2PJudge5Hold");
	}
	else if (timer == 86) {
		strcpy(translated, "On2PJudge6Hold");
	}
	else if (timer == 87) {
		strcpy(translated, "On2PJudge7Hold");
	}
	else if (timer == 88) {
		strcpy(translated, "On2PJudge8Hold");
	}
	else if (timer == 89) {
		strcpy(translated, "On2PJudge9Hold");
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
	else if (timer == 143) {
		strcpy(translated, "On1PLastNote");
	}
	else if (timer == 144) {
		strcpy(translated, "On2PLastNote");
	}
	else if (timer == 150) {
		strcpy(translated, "OnResult");
	}
	else {
		// unknown timer!
		// we can return '0', but I think it's better to return "UNKNOWN", for safety.
		return 0;
		//return "UNKNOWN";
	}
	return translated;
}

const char* _LR2SkinParser::TranslateButton(int code) {
	if (code == 1)
		strcpy(translated, "TogglePanel1()");
	else if (code == 2)
		strcpy(translated, "TogglePanel2()");
	else if (code == 3)
		strcpy(translated, "TogglePanel3()");
	else if (code == 4)
		strcpy(translated, "TogglePanel4()");
	else if (code == 5)
		strcpy(translated, "TogglePanel5()");
	else if (code == 6)
		strcpy(translated, "TogglePanel6()");
	else if (code == 7)
		strcpy(translated, "TogglePanel7()");
	else if (code == 8)
		strcpy(translated, "TogglePanel8()");
	else if (code == 9)
		strcpy(translated, "TogglePanel9()");
	else if (code == 10)
		strcpy(translated, "ChangeDiff()");
	else if (code == 11)
		strcpy(translated, "ChangeMode()");
	else if (code == 12)
		strcpy(translated, "ChangeSort()");
	/* change scene */
	else if (code == 13)
		strcpy(translated, "StartKeyConfig()");
	else if (code == 14)
		strcpy(translated, "StartSkinSetting()");
	else if (code == 15)
		strcpy(translated, "StartPlay()");
	else if (code == 16)
		strcpy(translated, "StartAutoPlay()");
	else if (code == 17)
		strcpy(translated, "StartTextView()");	// depreciated?
	/* 18: reset tag - is depreciated */
	else if (code == 19)
		strcpy(translated, "StartReplay()");
	// ignore FX button
	/* options */
	else if (code == 12)
		strcpy(translated, "ChangeSort()");
	/* options end */
	/* keyconfig */
	/* skinselect (used on skinsetting mode) */
	else
		return 0;
	return translated;
}

const char* _LR2SkinParser::TranslateSlider(int code) {
	if (code == 1) {
		strcpy(translated, "SelectBar");
	}
	else if (code == 2) {
		strcpy(translated, "HighSpeed1P");
	}
	else if (code == 3) {
		strcpy(translated, "HighSpeed2P");
	}
	else if (code == 4) {
		strcpy(translated, "Sudden1P");
	}
	else if (code == 5) {
		strcpy(translated, "Sudden2P");
	}
	else if (code == 6) {
		strcpy(translated, "PlayProgress");
	}
	// Lift isn't supported in LR2
	/* else (skin scroll, FX, etc ...) are all depreciated, so ignore. */
	else {
		return 0;
	}

	return translated;
}

const char* _LR2SkinParser::TranslateGraph(int code) {
	if (code == 1) {
		strcpy(translated, "PlayProgress");			// shares with number
	}
	else if (code == 2) {
		strcpy(translated, "SongLoadProgress");
	}
	else if (code == 3) {
		strcpy(translated, "SongLoadProgress");
	}
	else if (code == 5) {
		strcpy(translated, "BeginnerLevel_graph");	// graph only value
	}
	else if (code == 6) {
		strcpy(translated, "NormalLevel_graph");
	}
	else if (code == 7) {
		strcpy(translated, "HyperLevel_graph");
	}
	else if (code == 8) {
		strcpy(translated, "AnotherLevel_graph");
	}
	else if (code == 9) {
		strcpy(translated, "InsaneLevel_graph");
	}
	else if (code == 10) {
		strcpy(translated, "ExScore");
	}
	else if (code == 11) {
		strcpy(translated, "ExScoreEstimated");
	}
	else if (code == 12) {
		strcpy(translated, "HighScoreGhost");
	}
	else if (code == 13) {
		strcpy(translated, "HighScore");
	}
	else if (code == 14) {
		strcpy(translated, "TargetExScoreGhost");
	}
	else if (code == 15) {
		strcpy(translated, "TargetExScore");
	}
	else if (code == 20) {
		strcpy(translated, "ResultPerfectPercent");
	}
	else if (code == 21) {
		strcpy(translated, "ResultGreatPercent");
	}
	else if (code == 22) {
		strcpy(translated, "ResultGoodPercent");
	}
	else if (code == 23) {
		strcpy(translated, "ResultBadPercent");
	}
	else if (code == 24) {
		strcpy(translated, "ResultPoorPercent");
	}
	else if (code == 25) {
		strcpy(translated, "ResultMaxComboPercent");
	}
	else if (code == 26) {
		strcpy(translated, "ResultScorePercent");
	}
	else if (code == 27) {
		strcpy(translated, "ResultExScorePercent");
	}
	else if (code == 30) {
		strcpy(translated, "GhostPerfectPercent");
	}
	else if (code == 31) {
		strcpy(translated, "GhostGreatPercent");
	}
	else if (code == 32) {
		strcpy(translated, "GhostGoodPercent");
	}
	else if (code == 33) {
		strcpy(translated, "GhostBadPercent");
	}
	else if (code == 34) {
		strcpy(translated, "GhostPoorPercent");
	}
	else if (code == 35) {
		strcpy(translated, "GhostMaxComboPercent");
	}
	else if (code == 36) {
		strcpy(translated, "GhostScorePercent");
	}
	else if (code == 37) {
		strcpy(translated, "GhostExScorePercent");
	}
	// 40 ~ 47 highscore is depreciated; ignore
	else {
		return 0;
	}

	return translated;
}

const char* _LR2SkinParser::TranslateNumber(int code) {
	if (code == 10) {
		strcpy(translated, "1PSpeed");
	}
	else if (code == 11) {
		strcpy(translated, "2PSpeed");
	}
	else if (code == 12) {
		strcpy(translated, "JudgeTiming");
	}
	else if (code == 13) {
		strcpy(translated, "TargetRate");
	}
	else if (code == 14) {
		strcpy(translated, "1PSudden");
	}
	else if (code == 15) {
		strcpy(translated, "2PSudden");
	}/* LR2 doesn't support lift option
	else if (code == 14) {
		strcpy(translated, "1PLift");
	}
	else if (code == 15) {
		strcpy(translated, "2PLift");
	}*/
	else if (code == 20) {
		strcpy(translated, "FPS");
	}
	else if (code == 21) {
		strcpy(translated, "Year");
	}
	else if (code == 22) {
		strcpy(translated, "Month");
	}
	else if (code == 23) {
		strcpy(translated, "Day");
	}
	else if (code == 24) {
		strcpy(translated, "Hour");
	}
	else if (code == 25) {
		strcpy(translated, "Minute");
	}
	else if (code == 26) {
		strcpy(translated, "Second");
	}
	else if (code == 30) {
		strcpy(translated, "TotalPlayCount");
	}
	else if (code == 31) {
		strcpy(translated, "TotalClearCount");
	}
	else if (code == 32) {
		strcpy(translated, "TotalFailCount");
	}
	else if (code == 45) {
		strcpy(translated, "BeginnerLevel");
	}
	else if (code == 46) {
		strcpy(translated, "NormalLevel");
	}
	else if (code == 47) {
		strcpy(translated, "HyperLevel");
	}
	else if (code == 48) {
		strcpy(translated, "AnotherLevel");
	}
	else if (code == 49) {
		strcpy(translated, "InsaneLevel");
	}
	else if (code == 70) {
		strcpy(translated, "Score");
	}
	else if (code == 71) {
		strcpy(translated, "ExScore");
	}
	else if (code == 72) {
		strcpy(translated, "ExScore");
	}
	else if (code == 73) {
		strcpy(translated, "Rate");
	}
	else if (code == 74) {
		strcpy(translated, "TotalNotes");
	}
	else if (code == 75) {
		strcpy(translated, "MaxCombo");
	}
	else if (code == 76) {
		strcpy(translated, "MinBP");
	}
	else if (code == 77) {
		strcpy(translated, "PlayCount");
	}
	else if (code == 78) {
		strcpy(translated, "ClearCount");
	}
	else if (code == 79) {
		strcpy(translated, "FailCount");
	}
	else if (code == 80) {
		strcpy(translated, "PerfectCount");
	}
	else if (code == 81) {
		strcpy(translated, "GreatCount");
	}
	else if (code == 82) {
		strcpy(translated, "GoodCount");
	}
	else if (code == 83) {
		strcpy(translated, "BadCount");
	}
	else if (code == 84) {
		strcpy(translated, "PoorCount");
	}
	else if (code == 90) {
		strcpy(translated, "BPMMax");
	}
	else if (code == 91) {
		strcpy(translated, "BPMMin");
	}
	else if (code == 92) {
		strcpy(translated, "IRRank");
	}
	else if (code == 93) {
		strcpy(translated, "IRTotal");
	}
	else if (code == 94) {
		strcpy(translated, "IRRate");
	}
	else if (code == 95) {
		strcpy(translated, "RivalDiff");		// abs(HighExScore - HighExScoreRival)
	}
	/* during play */
	else if (code == 100) {
		strcpy(translated, "PlayScore");
	}
	else if (code == 101) {
		strcpy(translated, "PlayExScore");
	}
	else if (code == 102) {
		strcpy(translated, "PlayRate");
	}
	else if (code == 103) {
		strcpy(translated, "(PlayRate * 100) % 100");
	}
	else if (code == 104) {
		strcpy(translated, "PlayCombo");
	}
	else if (code == 105) {
		strcpy(translated, "PlayMaxCombo");
	}
	else if (code == 106) {
		strcpy(translated, "PlayTotalNotes");
	}
	else if (code == 107) {
		strcpy(translated, "PlayGrooveGuage");
	}
	else if (code == 108) {
		strcpy(translated, "PlayRivalDiff");
	}
	else if (code == 110) {
		strcpy(translated, "PlayPerfectCount");
	}
	else if (code == 111) {
		strcpy(translated, "PlayGreatCount");
	}
	else if (code == 112) {
		strcpy(translated, "PlayGoodCount");
	}
	else if (code == 113) {
		strcpy(translated, "PlayBadCount");
	}
	else if (code == 114) {
		strcpy(translated, "PlayPoorCount");
	}
	else if (code == 115) {
		strcpy(translated, "PlayTotalRate");	// estimated value
	}
	else if (code == 116) {
		strcpy(translated, "(PlayTotalRate * 100) / 100");
	}
	/* ghost */
	else if (code == 120) {
		strcpy(translated, "GhostScore");
	}
	else if (code == 121) {
		strcpy(translated, "GhostExScore");
	}
	else if (code == 122) {
		strcpy(translated, "GhostRate");
	}
	else if (code == 123) {
		strcpy(translated, "(GhostRate * 100) % 100");
	}
	else if (code == 124) {
		strcpy(translated, "GhostCombo");
	}
	else if (code == 125) {
		strcpy(translated, "GhostMaxCombo");
	}
	else if (code == 126) {
		strcpy(translated, "GhostTotalNotes");
	}
	else if (code == 127) {
		strcpy(translated, "GhostGrooveGuage");
	}
	else if (code == 128) {
		strcpy(translated, "GhostRivalDiff");
	}
	else if (code == 130) {
		strcpy(translated, "GhostPerfectCount");
	}
	else if (code == 131) {
		strcpy(translated, "GhostGreatCount");
	}
	else if (code == 132) {
		strcpy(translated, "GhostGoodCount");
	}
	else if (code == 133) {
		strcpy(translated, "GhostBadCount");
	}
	else if (code == 134) {
		strcpy(translated, "GhostPoorCount");
	}
	else if (code == 135) {
		strcpy(translated, "GhostTotalRate");	// estimated value
	}
	else if (code == 136) {
		strcpy(translated, "(GhostTotalRate * 100) / 100");
	}
	/*
	 * 150 ~ 158: TODO (useless?)
	 */
	else if (code == 160) {
		strcpy(translated, "PlayBPM");
	}
	else if (code == 161) {
		strcpy(translated, "PlayMinute");
	}
	else if (code == 162) {
		strcpy(translated, "PlaySecond");
	}
	else if (code == 163) {
		strcpy(translated, "PlayRemainMinute");
	}
	else if (code == 164) {
		strcpy(translated, "PlayRemainSecond");
	}
	else if (code == 165) {
		strcpy(translated, "PlayProgress");	// (%)
	}
	else if (code == 170) {
		strcpy(translated, "ResultExScoreBefore");
	}
	else if (code == 171) {
		strcpy(translated, "ResultExScoreNow");
	}
	else if (code == 172) {
		strcpy(translated, "ResultExScoreDiff");
	}
	else if (code == 173) {
		strcpy(translated, "ResultMaxComboBefore");
	}
	else if (code == 174) {
		strcpy(translated, "ResultMaxComboNow");
	}
	else if (code == 175) {
		strcpy(translated, "ResultMaxComboDiff");
	}
	else if (code == 176) {
		strcpy(translated, "ResultMinBPBefore");
	}
	else if (code == 177) {
		strcpy(translated, "ResultMinBPNow");
	}
	else if (code == 178) {
		strcpy(translated, "ResultMinBPDiff");
	}
	else if (code == 179) {
		strcpy(translated, "ResultIRRankNow");
	}
	else if (code == 180) {
		strcpy(translated, "ResultIRRankTotal");
	}
	else if (code == 181) {
		strcpy(translated, "ResultIRRankRate");
	}
	else if (code == 182) {
		strcpy(translated, "ResultIRRankBefore");
	}
	else if (code == 183) {
		strcpy(translated, "ResultRate");
	}
	else if (code == 184) {
		strcpy(translated, "(ResultRate * 100) / 100");
	}
	/* ignore IR Beta3: 200 ~ 250 */
	/* rival (in select menu) */
	else if (code == 270) {
		strcpy(translated, "RivalScore");
	}
	else if (code == 271) {
		strcpy(translated, "RivalExScore");
	}
	else if (code == 272) {
		strcpy(translated, "RivalRate");
	}
	else if (code == 273) {
		strcpy(translated, "(RivalRate * 100) % 100");
	}
	else if (code == 274) {
		strcpy(translated, "RivalCombo");
	}
	else if (code == 275) {
		strcpy(translated, "RivalMaxCombo");
	}
	else if (code == 276) {
		strcpy(translated, "RivalTotalNotes");
	}
	else if (code == 277) {
		strcpy(translated, "RivalGrooveGuage");
	}
	else if (code == 278) {
		strcpy(translated, "RivalRivalDiff");
	}
	else if (code == 280) {
		strcpy(translated, "RivalPerfectCount");
	}
	else if (code == 281) {
		strcpy(translated, "RivalGreatCount");
	}
	else if (code == 282) {
		strcpy(translated, "RivalGoodCount");
	}
	else if (code == 283) {
		strcpy(translated, "RivalBadCount");
	}
	else if (code == 284) {
		strcpy(translated, "RivalPoorCount");
	}
	/* 285 ~ is depreciated. ignore. */
	else {
		return 0;
	}

	return translated;
}

const char* _LR2SkinParser::TranslateText(int code) {
	if (code == 1) {
		strcpy(translated, "RivalName"); 
	}
	else if (code == 2) {
		strcpy(translated, "PlayerName");
	}
	else if (code == 10) {
		strcpy(translated, "Title");
	}
	else if (code == 11) {
		strcpy(translated, "Subtitle");
	}
	else if (code == 12) {
		strcpy(translated, "SumTitle");
	}
	else if (code == 13) {
		strcpy(translated, "Genre");
	}
	else if (code == 14) {
		strcpy(translated, "Artist");
	}
	else if (code == 15) {
		strcpy(translated, "SubArtist");
	}
	else if (code == 16) {
		strcpy(translated, "SearchTag");
	}
	else if (code == 17) {
		strcpy(translated, "PlayLevel");		// depreciated?
	}
	else if (code == 18) {
		strcpy(translated, "PlayDiff");			// depreciated?
	}
	else if (code == 19) {
		strcpy(translated, "PlayInsaneLevel");	// depreciated?
	}
	/*
	 * 20 ~ 30: for editing (depreciated/ignore?)
	 */
	else if (code == 40) {
		strcpy(translated, "KeySlot0");
	}
	else if (code == 41) {
		strcpy(translated, "KeySlot1");
	}
	else if (code == 42) {
		strcpy(translated, "KeySlot2");
	}
	else if (code == 43) {
		strcpy(translated, "KeySlot3");
	}
	else if (code == 44) {
		strcpy(translated, "KeySlot4");
	}
	else if (code == 45) {
		strcpy(translated, "KeySlot5");
	}
	else if (code == 46) {
		strcpy(translated, "KeySlot6");
	}
	else if (code == 47) {
		strcpy(translated, "KeySlot7");
	}
	/* Skin select window */
	else if (code == 50) {
		strcpy(translated, "SkinName");
	}
	else if (code == 51) {
		strcpy(translated, "SkinAuthor");
	}
	/* option */
	else if (code == 60) {
		strcpy(translated, "PlayMode");
	}
	else if (code == 61) {
		strcpy(translated, "PlaySort");
	}
	else if (code == 62) {
		strcpy(translated, "PlayDiff");
	}
	else if (code == 63) {
		strcpy(translated, "Random1P");
	}
	else if (code == 64) {
		strcpy(translated, "Random2P");
	}
	else if (code == 65) {
		strcpy(translated, "Guage1P");
	}
	else if (code == 66) {
		strcpy(translated, "Guage2P");
	}
	else if (code == 67) {
		strcpy(translated, "Assist1P");
	}
	else if (code == 68) {
		strcpy(translated, "Assist2P");
	}
	else if (code == 69) {
		strcpy(translated, "Battle");		// depreciated?
	}
	else if (code == 70) {
		strcpy(translated, "Flip");			// depreciated?
	}
	else if (code == 71) {
		strcpy(translated, "ScoreGraph");	// depreciated?
	}
	else if (code == 72) {
		strcpy(translated, "Ghost");
	}
	else if (code == 74) {
		strcpy(translated, "ScrollType");
	}
	else if (code == 75) {
		strcpy(translated, "BGASize");		// depreciated
	}
	else if (code == 76) {
		strcpy(translated, "IsBGA");		// depreciated?
	}/*
	screen color: depreciated
	else if (code == 60) {
		strcpy(translated, "ScreenColor");
	}*/
	else if (code == 78) {
		strcpy(translated, "VSync");
	}
	else if (code == 79) {
		strcpy(translated, "ScreenMode");	// full/window
	}
	else if (code == 80) {
		strcpy(translated, "AutoJudge");
	}
	else if (code == 81) {
		strcpy(translated, "ReplaySave");
	}
	// ignore trial lines / ignore effects
	/*
	 * 100 ~ is skin related; ignore. 
	 * skin option select will be done in scrollbar / overflow option
	 * (TODO)
	 */
	else {
		return 0;
	}

	return translated;
}

void _LR2SkinParser::Clear() {
	s = 0;
	line_total = 0;
	cur_e = 0;
	image_cnt = 0;
	font_cnt = 0;
	filter_to_optionname.clear();
	texturefont_id.clear();
	memset(line_args, 0, sizeof(line_args));
}

// ----------------------- LR2Skin part end ------------------------
