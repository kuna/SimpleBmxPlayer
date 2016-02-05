#include "skin.h"
#include "skinutil.h"
#include "skintexturefont.h"
using namespace SkinUtil;
using namespace tinyxml2;
#include <sstream>

// temp use
namespace {
	char translated[1024];
	const char empty_str_[] = "";
}

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

// utility macros
#define ADDCHILD(base, name)\
	(base)->LinkEndChild((base)->GetDocument()->NewElement(name))
#define ADDTEXT(base, name, val)\
	((XMLElement*)ADDCHILD(base, name))->SetText(val);

// ---------------------------------------------------------------

bool _LR2SkinParser::ParseLR2Skin(const char *filepath, Skin *s) {
	// fill basic information to skin
	this->s = s;

	strcpy(s->filepath, filepath);
	XMLComment *cmt = s->skinlayout.NewComment("Auto-generated code.\n"
		"Converted from LR2Skin.\n"
		"LR2 project origins from lavalse, All rights reserved.");
	s->skinlayout.LinkEndChild(cmt);
	/* 4 elements are necessary */
	XMLElement *skin = s->skinlayout.NewElement("skin");
	s->skinlayout.LinkEndChild(skin);

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
	strcpy(this->filepath, filepath);

	char line[MAX_LINE_CHARACTER_];
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
		/*if (strncmp("#INCLUDE", p, 8) == 0) {
			char *np = strchr(p + 9, ',');
			if (np) *np = 0;
			std::string relpath = p + 9;
			std::string basepath = filepath;
			ConvertLR2PathToRelativePath(relpath);
			GetParentDirectory(basepath);
			ConvertRelativePathToAbsPath(relpath, basepath);
			linebufferpos += LoadSkin(relpath.c_str(), linebufferpos + 1);
		}
		else {*/
		line_v_ line__;
		strcpy(line__.line__, line);
		lines_.push_back(line__);
		//}
		linebufferpos++;
	}

	fclose(f);

	// parse argument
	for (int i = 0; i < lines_.size(); i++) {
		args_v_ line_args__;
		line_v_* line__stored_ = &lines_[i];
		ParseSkinLineArgument(line__stored_->line__, line_args__.args__);
		line_args_.push_back(line_args__);
	}

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

void _LR2SkinParser::ParseSkinLineArgument(char *p, const char **args) {
	// first element is element name
	args[0] = p;
	int i;
	for (i = 1; i < MAX_ARGS_COUNT_; i++) {
		p = strchr(p, ',');
		if (!p)
			break;
		*p = 0;
		args[i] = (++p);
	}
	// for safety, pack left argument as ""
	for (; i < MAX_ARGS_COUNT_; i++) {
		args[i] = empty_str_;
	}
}

/* a simple private macro for PLAYLANE (reset position) */
void MakeFrameRelative(int x, int y, XMLElement *frame) {
	frame->SetAttribute("x", frame->IntAttribute("x") - x);
	frame->SetAttribute("y", frame->IntAttribute("y") - y);
}
void MakeRelative(int x, int y, XMLElement *e) {
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		XMLElement *frame = dst->FirstChildElement("frame");
		while (frame) {
			MakeFrameRelative(x, y, frame);
			frame = frame->NextSiblingElement("frame");
		}
		dst = e->NextSiblingElement("DST");
	}
}


int _LR2SkinParser::ParseSkinLine(int line) {
	// get current line's string & argument
	args_read_ args;			// contains linebuffer's address
	args = line_args_[line].args__;
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
		if (condition_level > 0)
			cur_e = condition_element[--condition_level];
		else
			printf("Invalid #ENDIF (%d)\n", line);
		return line + 1;
	}
	if (!cur_e)
		return line + 1;
	if (CMD_IS("#ELSE")) {
		// #ELSE: find last #IF clause, and copy condition totally.
		// not perfect, but maybe we can make a deal :)
		XMLElement *prev_if = condition_element[condition_level - 1]->LastChildElement("if");
		if (prev_if) {
			XMLElement *group = s->skinlayout.NewElement("ifnot");
			group->SetAttribute("condition", prev_if->Attribute("condition"));
			cur_e = condition_element[--condition_level];
			cur_e->LinkEndChild(group);
			condition_element[++condition_level] = cur_e = group;
			condition_status[condition_level]++;
		}
		else {
			// we tend to ignore #ELSE clause ... it's wrong.
			cur_e = 0;
		}
		return line + 1;
	}
	else if (CMD_IS("#IF") || CMD_IS("#ELSEIF")) {
		// #ELSEIF: think as new #IF clause
		XMLElement *group = s->skinlayout.NewElement("if");
		if (CMD_IS("#IF")) {
			condition_level++;
			condition_status[condition_level] = 0;
		}
		condition_element[condition_level] = group;
		condition_status[condition_level]++;
		ConditionAttribute cls;
		for (int i = 1; i < 50 && args[i]; i++) {
			const char *c = INT(args[i])?TranslateOPs(INT(args[i])):0;
			if (c) cls.AddCondition(c);
		}
		group->SetAttribute("condition", cls.ToString());
		// get parent object and link new group to there
		cur_e = condition_element[condition_level - 1];	
		cur_e->LinkEndChild(group);
		cur_e = group;
		return line + 1;
	}

	/*
	* header/metadata parsing
	*/
	if (CMD_IS("#INCLUDE")) {
		XMLElement *e = (XMLElement*)ADDCHILD(cur_e, "include");

		std::string relpath = args[1];
		std::string basepath = filepath;
		ConvertLR2PathToRelativePath(relpath);
		//GetParentDirectory(basepath);
		//ConvertRelativePathToAbsPath(relpath, basepath);

		e->SetAttribute("path", relpath.c_str());
	}
	else if (CMD_IS("#CUSTOMOPTION")) {
		XMLElement *option = FindElement(cur_e, "option", &s->skinlayout);
		XMLElement *customoption = s->skinlayout.NewElement("customswitch");
		option->LinkEndChild(customoption);

		std::string name_safe = args[1];
		ReplaceString(name_safe, " ", "_");
		customoption->SetAttribute("name", name_safe.c_str());
		int option_intvalue = atoi(args[2]);
		for (const char **p = args + 3; *p != 0 && strlen(*p) > 0; p++) {
			XMLElement *options = s->skinlayout.NewElement("option");
			options->SetAttribute("name", *p);
			options->SetAttribute("value", option_intvalue);
			option_intvalue++;
			customoption->LinkEndChild(options);
		}
	}
	else if (CMD_IS("#CUSTOMFILE")) {
		XMLElement *option = FindElement(cur_e, "option", &s->skinlayout);
		XMLElement *customfile = s->skinlayout.NewElement("customfile");
		option->LinkEndChild(customfile);

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

		// register to filter_to_optionname for image change
		AddPathToOption(args[2], name_safe);
		XMLComment *cmt = s->skinlayout.NewComment(args[2]);
		option->LinkEndChild(cmt);
	}
	else if (CMD_IS("#INFORMATION")) {
		// set skin's metadata
		XMLElement *info = (XMLElement*)ADDCHILD(cur_e, "info");
		ADDTEXT(info, "width", 1280);
		ADDTEXT(info, "height", 720);
		int type_ = INT(args[1]);
		if (type_ == 5) {
			ADDTEXT(info, "type", "Select");
		}
		else if (type_ == 6) {
			ADDTEXT(info, "type", "Decide");
		}
		else if (type_ == 7) {
			ADDTEXT(info, "type", "Result");
		}
		else if (type_ == 8) {
			ADDTEXT(info, "type", "KeyConfig");
		}
		/** @comment skinselect / soundselect are all depreciated, integrated into option. */
		else if (type_ == 9) {
			ADDTEXT(info, "type", "SkinSelect");
		}
		else if (type_ == 10) {
			ADDTEXT(info, "type", "SoundSelect");
		}
		/** @comment end */
		else if (type_ == 12) {
			ADDTEXT(info, "type", "Play");
			ADDTEXT(info, "key", 15);
		}
		else if (type_ == 13) {
			ADDTEXT(info, "type", "Play");
			ADDTEXT(info, "key", 17);
		}
		else if (type_ == 15) {
			ADDTEXT(info, "type", "CourseResult");
		}
		else if (type_ < 5) {
			ADDTEXT(info, "type", "Play");
			int key_ = 7;
			switch (type_) {
			case 0:
				// 7key
				key_ = 7;
				break;
			case 1:
				// 9key
				key_ = 5;
				break;
			case 2:
				// 14key
				key_ = 14;
				break;
			case 3:
				key_ = 10;
				break;
			case 4:
				key_ = 9;
				break;
			}
			ADDTEXT(info, "key", key_);
		}
		else {
			printf("[ERROR] unknown type of lr2skin(%d). consider as 7Key Play.\n", type_);
			type_ = 0;
		}

		ADDTEXT(info, "name", args[2]);
		ADDTEXT(info, "author", args[3]);
	}
	else if (CMD_IS("#IMAGE")) {
		/*
		 * Sometimes resource is added in IF form
		 * then ONLY allow first one
		 */
		if (condition_level > 0 && condition_status[condition_level] > 1)
			return line + 1;
		XMLElement *resource = s->skinlayout.FirstChildElement("skin");
		XMLElement *image = s->skinlayout.NewElement("image");
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

		resource->InsertFirstChild(image);
	}
	else if (CMD_IS("#FONT")) {
		printf("#FONT is depreciated option, ignore.\n");
	}
	else if (CMD_IS("#LR2FONT")) {
		// we don't use bitmap fonts
		// So if cannot found, we'll going to use default font/texture.
		// current font won't support TTF, so basically we're going to use default font.
		XMLElement *resource = s->skinlayout.FirstChildElement("skin");
		XMLElement *font = s->skinlayout.NewElement("font");
		font->SetAttribute("name", font_cnt++);
		font->SetAttribute("path", "default");
		int size = 18;
		if (strstr(args[1], "small")) size = 15;
		if (strstr(args[1], "title")
			|| strstr(args[1], "big")
			|| strstr(args[1], "large")) 
			size = 42;
		if (size > 20)
			font->SetAttribute("texturepath", "default");
		font->SetAttribute("size", size);
#if 0
		/*
		 * these are available in #FONT, not #LR2FONT
		 */
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
#endif
		font->SetAttribute("border", size / 20 + 1);
		resource->InsertFirstChild(font);
	}
	else if (CMD_IS("#SETOPTION")) {
		// this clause is translated during render tree construction
		XMLElement *setoption = s->skinlayout.NewElement("lua");
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
		obj = s->skinlayout.NewElement("sprite");
		int resid = INT(args[2]);	// COMMENT: check out for pre-occupied resid
		switch (resid) {
		case 100:
			obj->SetAttribute("resid", "_stagefile");
			break;
		case 101:
			obj->SetAttribute("resid", "_backbmp");
			break;
		case 102:
			obj->SetAttribute("resid", "_banner");
			break;
		case 110:
			obj->SetAttribute("resid", "_black");
			break;
		case 111:
			obj->SetAttribute("resid", "_white");
			break;
		default:
			obj->SetAttribute("resid", resid);
			break;
		}
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
		sop1 = INT(args[11]);
		sop2 = INT(args[12]);
		sop3 = INT(args[13]);

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
		XMLElement *dst = s->skinlayout.NewElement("DST");
		obj->LinkEndChild(dst);
		for (int nl = line + 1; nl < line_total; nl++) {
			args = line_args_[nl].args__;
			if (!args[0]) continue;
			if (CMD_IS("#ENDIF"))
				continue;			// we can ignore #ENDIF command, maybe
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
			XMLElement *frame = s->skinlayout.NewElement("frame");
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
			dst->LinkEndChild(frame);
		}
		// set common draw attribute
		dst->SetAttribute("acc", acc);
		if (blend > 1)
			dst->SetAttribute("blend", blend);
		if (looptime >= 0)
			dst->SetAttribute("loop", looptime);
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

		/*
		* If object is select screen panel dependent(timer/op code 2x),
		* then add object to there (ease of control)
		* TODO: take care of 3x objects (OnPanelClose)
		*/
#define CHECK_PANEL(v) (op1 == (v) || op2 == (v) || op3 == (v) || timer == (v))
		if (CHECK_PANEL(21))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel1", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(22))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel2", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(23))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel3", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(24))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel4", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(25))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel5", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(26))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel6", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(27))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel7", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(28))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel8", &s->skinlayout)->LinkEndChild(obj);
		else if (CHECK_PANEL(29))
			FindElementWithAttribute(cur_e, "canvas", "id", "panel9", &s->skinlayout)->LinkEndChild(obj);
		else
			cur_e->LinkEndChild(obj);
		

		/*
		 * Check out for some special object (which requires #DST object)
		 * COMMENT: most of them behaves like #IMAGE object.
		 */
		// reset arguments to figure out about object
		args = line_args_[line].args__;
		int objectid = INT(args[1]);

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
					XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", 0, &s->skinlayout);
					MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
					playarea->LinkEndChild(obj);
				}
				else if (//(timer >= 60 && timer < 70) ||
					(timer >= 80 && timer < 90) ||
					(timer >= 110 && timer < 120) ||
					(timer >= 130 && timer < 140)) {
					// P2
					XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", 1, &s->skinlayout);
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
			obj->SetName("bga");
			// set bga side & remove redundant tag
			// (LR2 doesn't support 'real' battle mode, so no side attribute.)
			obj->SetAttribute("side", 0);
			obj->DeleteAttribute("resid");
		}
		else if (OBJTYPE_IS("NUMBER")) {
			obj->SetName("number");
			ProcessNumber(obj, sop1, sop2, sop3);
		}
		else if (OBJTYPE_IS("SLIDER")) {
			// change tag to slider and add attr
			obj->SetName("slider");
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
			obj->SetName("text");
			if (TranslateText(INT(args[3])))
				obj->SetAttribute("value", TranslateText(INT(args[3])));
			obj->SetAttribute("align", args[4]);
			obj->SetAttribute("edit", args[5]);
		}
		else if (OBJTYPE_IS("BARGRAPH")) {
			obj->SetName("graph");
			if (TranslateGraph(sop1))
				obj->SetAttribute("value", TranslateGraph(sop1));
			obj->SetAttribute("direction", sop2);
		}
		else if (OBJTYPE_IS("BUTTON")) {
			// TODO: onclick event
			obj->SetName("button");
		}
		/* 
		 * some special object (PLAY lane object) 
		 */
		else if (OBJTYPE_IS("GROOVEGAUGE")) {
			int side = INT(args[1]);
			int addx = sop1;
			int addy = sop2;
			obj->SetAttribute("side", side);
			obj->SetAttribute("addx", addx);
			obj->SetAttribute("addy", addy);
			// process SRC and make new SRC elements
			int x = obj->IntAttribute("x");
			int y = obj->IntAttribute("y");
			int w = obj->IntAttribute("w");
			int h = obj->IntAttribute("h");
			int divx = obj->IntAttribute("divx");
			int divy = obj->IntAttribute("divy");
			int c = divx * divy;
			int dw = w / divx;
			int dh = h / divy;
			XMLElement *active, *inactive;
			active = s->skinlayout.NewElement("SRC_GROOVE_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_GROOVE_INACTIVE");
			active->SetAttribute("x", x + dw * (1 % divx));
			active->SetAttribute("y", y + dh * (1 / divx % divy));
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (3 % divx));
			inactive->SetAttribute("y", y + dh * (3 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			active = s->skinlayout.NewElement("SRC_HARD_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_HARD_INACTIVE");
			active->SetAttribute("x", x);
			active->SetAttribute("y", y);
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (2 % divx));
			inactive->SetAttribute("y", y + dh * (2 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			active = s->skinlayout.NewElement("SRC_EX_ACTIVE");
			inactive = s->skinlayout.NewElement("SRC_EX_INACTIVE");
			active->SetAttribute("x", x);
			active->SetAttribute("y", y);
			active->SetAttribute("w", dw);
			active->SetAttribute("h", dh);
			inactive->SetAttribute("x", x + dw * (2 % divx));
			inactive->SetAttribute("y", y + dh * (2 / divx % divy));
			inactive->SetAttribute("w", dw);
			inactive->SetAttribute("h", dh);
			obj->LinkEndChild(active);
			obj->LinkEndChild(inactive);

			// remove obsolete SRC
			obj->DeleteAttribute("x");
			obj->DeleteAttribute("y");
			obj->DeleteAttribute("w");
			obj->DeleteAttribute("h");
			obj->DeleteAttribute("divx");
			obj->DeleteAttribute("divy");
			obj->SetName("groovegauge");
		}
		else if (OBJTYPE_IS("JUDGELINE")) {
			obj->SetName("judgeline");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("LINE")) {
			obj->SetName("line");
			XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
			MakeRelative(playarea->IntAttribute("x"), playarea->IntAttribute("y"), obj);
			playarea->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("ONMOUSE")) {
			// depreciated/ignore
			// TODO: support this by SRC_HOVER tag.
			printf("#XXX_ONMOUSE command is depreciated, ignore. (%dL) \n", line);
		}
		else {
			printf("Unknown General Object (%s), consider as IMAGE. (%dL)\n", args[0] + 5, line);
		}

		// return new line
		return line+1;
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
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu");
		selectmenu->SetAttribute("center", line_args_[line].args__[1]);
	}
	else if (CMD_IS("#BAR_AVAILABLE")) {
		// depreciated, not parse
		printf("#BAR_AVAILABLE - depreciated, Ignore.\n");
	}
	/*
	 * PLAY part 
	 */
	else if (CMD_STARTSWITH("#DST_NOTE", 9)) {
		args_read_ args = line_args_[line].args__;
		int objectid = INT(args[1]);
		XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid / 10, &s->skinlayout);
		XMLElement *lane = FindElementWithAttribute(playarea, "note", "index", objectid, &s->skinlayout);
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
		printf("Unknown Type: %s (%dL) - Ignore.\n", args[0], line);
	}

	// parse next line
	return line + 1;
}

// comment: maybe I need to process it with namespace ...?
void _LR2SkinParser::ProcessNumber(XMLElement *obj, int sop1, int sop2, int sop3) {
	// just convert SRC to texturefont ...
	ConvertToTextureFont(obj);
	/*
	* Number object will act just like a extended-string object.
	* If no value, then it'll just show '0' value.
	*/
	if (TranslateNumber(sop1))
		obj->SetAttribute("value", TranslateNumber(sop1));
	/*
	* LR2's NUMBER alignment is a bit strange ...
	* why is it different from string's alignment ... fix it
	*/
	if (sop2 == 2)
		sop2 = 1;
	else if (sop2 == 1)
		sop2 = 2;
	else if (sop2 == 0)
		sop2 = 0;
	obj->SetAttribute("align", sop2);
	/*
	* if type == 11 or 24, then set length
	* if type == 24, then set '24mode' (only for LR2 - depreciated supportance)
	* (If you want to implement LR2-like font, then you may have to make 2 type of texturefont SRC -
	* plus and minus - with proper condition.)
	*/
	if (sop3)
		obj->SetAttribute("length", sop3);

	// remove some attrs
	obj->DeleteAttribute("x");
	obj->DeleteAttribute("y");
	obj->DeleteAttribute("w");
	obj->DeleteAttribute("h");
	obj->DeleteAttribute("divx");
	obj->DeleteAttribute("divy");
	obj->DeleteAttribute("cycle");
}

/*
 * process commands about lane
 * if not lane, return 0
 * if lane, return next parsed line
 */
int _LR2SkinParser::ProcessLane(XMLElement *src, int line, int resid) {
	args_read_ args = line_args_[line].args__;
	int objectid = INT(args[1]);

#define SETNOTE(name)\
	XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid / 10, &s->skinlayout);\
	XMLElement *lane = FindElementWithAttribute(playarea, "note", "index", objectid, &s->skinlayout);\
	lane->SetAttribute("resid", src->Attribute("resid"));\
	src->DeleteAttribute("resid");\
	src->SetName(name);\
	lane->LinkEndChild(src);\
	return line + 1;
	if (OBJTYPE_IS("NOTE")) {
		SETNOTE("SRC_NOTE");
	}
	else if (OBJTYPE_IS("LN_END")) {
		SETNOTE("SRC_LN_END");
	}
	else if (OBJTYPE_IS("LN_BODY")) {
		SETNOTE("SRC_LN_BODY");
	}
	else if (OBJTYPE_IS("LN_START")) {
		SETNOTE("SRC_LN_START");
	}
	else if (OBJTYPE_IS("MINE")) {
		SETNOTE("SRC_MINE");
	}
	if (OBJTYPE_IS("AUTO_NOTE")) {
		SETNOTE("SRC_AUTO_NOTE");
	}
	else if (OBJTYPE_IS("AUTO_LN_END")) {
		SETNOTE("SRC_AUTO_LN_END");
	}
	else if (OBJTYPE_IS("AUTO_LN_BODY")) {
		SETNOTE("SRC_AUTO_LN_BODY");
	}
	else if (OBJTYPE_IS("AUTO_LN_START")) {
		SETNOTE("SRC_AUTO_LN_START");
	}
	else if (OBJTYPE_IS("AUTO_MINE")) {
		SETNOTE("SRC_AUTO_MINE");
	}
	else if (OBJTYPE_IS("JUDGELINE")) {
		XMLElement *playarea = FindElementWithAttribute(cur_e, "notefield", "side", objectid, &s->skinlayout);
		// find DST object to set Lane attribute
		for (int _l = line + 1; _l < line_total; _l++) {
			if (strcmp(line_args_[_l].args__[0], "#DST_JUDGELINE") == 0 &&
				INT(line_args_[_l].args__[1]) == objectid) {
				int x = INT(line_args_[_l].args__[3]);
				int y = 0;
				int w = INT(line_args_[_l].args__[5]);
				int h = INT(line_args_[_l].args__[4]);
				playarea->SetAttribute("x", x);
				playarea->SetAttribute("y", y);
				playarea->SetAttribute("w", w);
				playarea->SetAttribute("h", h);
				XMLElement *dst = FindElement(playarea, "DST", &s->skinlayout);
				XMLElement *frame = FindElement(dst, "frame", &s->skinlayout);
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

std::string _getcomboconditionstring(int player, int level) {
	char buf[256];
	sprintf(buf, "OnP%dJudge", player);

	switch (level) {
	case 0:
		return std::string(buf) + "NPoor";
		break;
	case 1:
		return std::string(buf) + "Poor";
		break;
	case 2:
		return std::string(buf) + "Bad";
		break;
	case 3:
		return std::string(buf) + "Good";
		break;
	case 4:
		return std::string(buf) + "Great";
		break;
	case 5:
		return std::string(buf) + "Perfect";
		break;
	}
}
int __comboy = 0;
int __combox = 0;
int _LR2SkinParser::ProcessCombo(XMLElement *obj, int line) {
	args_read_ args = line_args_[line].args__;
	int objectid = INT(args[1]);
	int sop1 = 0, sop2 = 0, sop3 = 0;
	if (args[11]) sop1 = INT(args[11]);
	if (args[12]) sop2 = INT(args[12]);
	if (args[13]) sop3 = INT(args[13]);

#define GETCOMBOOBJ(side)\
	std::string cond = _getcomboconditionstring(side, objectid);\
	XMLElement *playcombo = FindElementWithAttribute(cur_e, "combo", "condition", cond.c_str(), &s->skinlayout);
	if (OBJTYPE_IS("NOWJUDGE_1P")) {
		GETCOMBOOBJ(1);
		obj->SetName("sprite");
		playcombo->LinkEndChild(obj);
		__comboy = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("y");
		__combox = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("x");
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_1P")) {
		GETCOMBOOBJ(1);
		obj->SetName("number");
		ProcessNumber(obj, 0, 0, 0);
		obj->SetAttribute("value", "P1Combo");
		obj->SetAttribute("align", 1);
		for (XMLElement *e = obj->FirstChildElement("DST")->FirstChildElement("frame"); e;) {
			e->SetAttribute("y", __comboy);
			e->SetAttribute("x", e->IntAttribute("x") + __combox);
			e->SetAttribute("w", 0);
			e = e->NextSiblingElement("frame");
		}
		playcombo->LinkEndChild(obj);
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWJUDGE_2P")) {
		GETCOMBOOBJ(2);
		obj->SetName("sprite");
		playcombo->LinkEndChild(obj);
		__comboy = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("y");
		__combox = obj->FirstChildElement("DST")->FirstChildElement("frame")->IntAttribute("x");
		return line + 1;
	}
	else if (OBJTYPE_IS("NOWCOMBO_2P")) {
		GETCOMBOOBJ(2);
		obj->SetName("number");
		ProcessNumber(obj, 0, 0, 0);
		obj->SetAttribute("value", "P2Combo");
		obj->SetAttribute("align", 1);
		for (XMLElement *e = obj->FirstChildElement("DST")->FirstChildElement("frame"); e;) {
			e->SetAttribute("y", __comboy);
			e->SetAttribute("x", e->IntAttribute("x") + __combox);
			e->SetAttribute("w", 0);
			e = e->NextSiblingElement("frame");
		}
		playcombo->LinkEndChild(obj);
		return line + 1;
	}

	// not a combo object
	return 0;
}

int _LR2SkinParser::ProcessSelectBar(XMLElement *obj, int line) {
	args_read_ args = line_args_[line].args__;
	int objectid = INT(args[1]);

	// select menu part
	if (!OBJTYPE_IS("BARGRAPH") && CMD_STARTSWITH("#SRC_BAR", 8)) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
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
			obj->SetName("flash");
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_TITLE")) {
			obj->SetName("title");
			obj->SetAttribute("type", objectid);	// new: 1
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LEVEL")) {
			obj->SetName("level");
			obj->SetAttribute("type", objectid);	// difficulty
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_LAMP")) {
			obj->SetName("lamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_MY_LAMP")) {
			obj->SetName("mylamp");
			obj->SetAttribute("type", objectid);	// clear
			selectmenu->LinkEndChild(obj);
		}
		else if (OBJTYPE_IS("BAR_RIVAL_LAMP")) {
			obj->SetName("rivallamp");
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
	args_read_ args = line_args_[line].args__;
	int objectid = INT(args[1]);
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
	if (CMD_IS("#DST_BAR_BODY_ON")) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "position", &s->skinlayout);
		XMLElement *bodyoff = FindElement(position, "bar");
		if (bodyoff) {
			/*
			* DST_SELECTED: only have delta_x, delta_y value
			*/
			XMLElement *frame = FindElement(bodyoff, "frame");
			position->SetAttribute("delta_x", INT(args[3]) - frame->IntAttribute("x"));
		}
		return line + 1;
	}
	else if (CMD_IS("#DST_BAR_BODY_OFF")) {
		XMLElement *selectmenu = FindElement(cur_e, "selectmenu", &s->skinlayout);
		XMLElement *position = FindElement(selectmenu, "position", &s->skinlayout);
		XMLElement *bodyoff = FindElementWithAttribute(position, "bar", "index", INT(args[1]), &s->skinlayout);
		XMLElement *frame = s->skinlayout.NewElement("frame");
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
		obj->SetAttribute("mode24", true);
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
	const char* timer = obj->Attribute("timer");
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
	XMLElement *resource = s->skinlayout.FirstChildElement("skin");
	XMLElement *img = FindElementWithAttribute(resource, "image", "name", obj->Attribute("resid"));
	// create font data
	SkinTextureFont tfont;
	tfont.AddImageSrc(img->Attribute("path"));
	tfont.SetCycle(obj->IntAttribute("cycle"));
	if (timer) tfont.SetTimer(timer);
	tfont.SetFallbackWidth(dw);
	char glyphs[] = "0123456789*+ABCDEFGHIJ#-";
	for (int r = 0; r < repcnt; r++) {
		for (int i = 0; i < fonttype; i++) {
			int gi = i + r * fonttype;
			int cx = gi % divx;
			int cy = gi / divx;
			tfont.AddGlyph(glyphs[i], 0, x + dw * cx, y + dh * cy, dw, dh);
		}
	}
	tfont.SaveToText(out);
	out = "\n# Auto-generated texture font data by SkinParser\n" + out;

	// register to LR2 resource
	XMLElement *res = s->skinlayout.FirstChildElement("skin");
	XMLElement *restfont = s->skinlayout.NewElement("texturefont");
	restfont->SetAttribute("name", font_cnt-1);		// create new texture font
	restfont->SetAttribute("type", "1");			// for LR2 font type. (but not decided for other format, yet.)
	restfont->SetText(out.c_str());
	res->InsertFirstChild(restfont);

	return font_cnt-1;
}

#define SETOPTION(s) (strcat_s(translated, 1024, s))
#define SETNEGATIVEOPTION(s) \
if (translated[0] == '!') translated[0] = 0;\
else strcpy_s(translated, 1024, "!");\
strcat_s(translated, 1024, s);
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
	else if (op > 0 && op < 200) {
		if (op == 1) {
			SETOPTION("IsSelectBarFolder");
		}
		else if (op == 2) {
			SETOPTION("IsSelectBarSong");
		}
		else if (op == 3) {
			SETOPTION("IsSelectBarCourse");
		}
		else if (op == 4) {
			SETOPTION("IsSelectBarNewCourse");
		}
		else if (op == 5) {
			SETOPTION("IsSelectBarPlayable");
		}
		else if (op == 10) {
			SETOPTION("IsDoublePlay");
		}
		else if (op == 11) {
			SETOPTION("IsBattlePlay");
		}
		else if (op == 12) {
			SETOPTION("IsDoublePlay");	// this includes battle
		}
		else if (op == 13) {
			SETOPTION("IsBattlePlay");	// this includes ghost battle
		}
		else if (op == 20) {
			SETOPTION("OnPanel");
		}
		else if (op == 21) {
			SETOPTION("OnPanel1");
		}
		else if (op == 22) {
			SETOPTION("OnPanel2");
		}
		else if (op == 23) {
			SETOPTION("OnPanel3");
		}
		else if (op == 24) {
			SETOPTION("OnPanel4");
		}
		else if (op == 25) {
			SETOPTION("OnPanel5");
		}
		else if (op == 26) {
			SETOPTION("OnPanel6");
		}
		else if (op == 27) {
			SETOPTION("OnPanel7");
		}
		else if (op == 28) {
			SETOPTION("OnPanel8");
		}
		else if (op == 29) {
			SETOPTION("OnPanel9");
		}
		else if (op == 30) {
			SETOPTION("IsBGANormal");		// Depreciated; won't be used
		}
		else if (op == 31) {
			SETOPTION("IsBGA");
		}
		else if (op == 32) {
			SETNEGATIVEOPTION("IsAutoPlay");
		}
		else if (op == 33) {
			SETOPTION("IsAutoPlay");
		}
		else if (op == 34) {
			SETOPTION("IsGhostOff");			// hmm ...
		}
		else if (op == 35) {
			SETOPTION("IsGhostA");
		}
		else if (op == 36) {
			SETOPTION("IsGhostB");
		}
		else if (op == 37) {
			SETOPTION("IsGhostC");
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
			SETOPTION("IsBGA");
		}
		else if (op == 42) {
			SETOPTION("IsP1GrooveGauge");
		}
		else if (op == 43) {
			SETOPTION("IsP1HardGauge");
		}
		else if (op == 44) {
			SETOPTION("IsP2GrooveGauge");
		}
		else if (op == 45) {
			SETOPTION("IsP2HardGauge");
		}
		else if (op == 46) {
			SETOPTION("IsDiffFiltered");		// on select menu; but depreciated?
		}
		else if (op == 47) {
			SETNEGATIVEOPTION("IsDifficultyFilter");
		}
		else if (op == 50) {
			SETNEGATIVEOPTION("IsOnline");
		}
		else if (op == 51) {
			SETOPTION("IsOnline");
		}
		else if (op == 52) {
			SETNEGATIVEOPTION("IsExtraMode");			// DEMO PLAY
		}
		else if (op == 53) {
			SETOPTION("IsExtraMode");
		}
		else if (op == 54) {
			SETNEGATIVEOPTION("IsP1AutoSC");
		}
		else if (op == 55) {
			SETOPTION("IsP1AutoSC");
		}
		else if (op == 56) {
			SETNEGATIVEOPTION("IsP2AutoSC");
		}
		else if (op == 57) {
			SETOPTION("IsP2AutoSC");
		}
		else if (op == 60) {
			SETNEGATIVEOPTION("IsRecordable");
		}
		else if (op == 61) {
			SETOPTION("IsRecordable");
		}
		else if (op == 62) {
			SETNEGATIVEOPTION("IsRecordable");
		}
		else if (op == 63) {
			SETOPTION("IsEasyClear");
		}
		else if (op == 64) {
			SETOPTION("IsGrooveClear");
		}
		else if (op == 65) {
			SETOPTION("IsHardClear");
		}/* NO EXH in LR2
		else if (op == 66) {
		strcat(translated, "IsEXHClear");
		}*/
		else if (op == 66) {
			SETOPTION("IsFCClear");
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
			SETOPTION("IsBeginnerSparkle");
		}
		else if (op == 76) {
			SETOPTION("IsNormalSparkle");
		}
		else if (op == 77) {
			SETOPTION("IsHyperSparkle");
		}
		else if (op == 78) {
			SETOPTION("IsAnotherSparkle");
		}
		else if (op == 79) {
			SETOPTION("IsInsaneSparkle");
		}
		else if (op == 80) {
			SETOPTION("OnSongLoading");
		}
		else if (op == 81) {
			SETOPTION("OnSongLoadingEnd");
		}
		else if (op == 84) {
			SETOPTION("OnSongReplay");
		}
		else if (op == 90) {
			SETOPTION("OnResultClear");
		}
		else if (op == 91) {
			SETOPTION("OnResultFail");
		}
		else if (op == 150) {
			SETOPTION("OnDiffNone");			// I suggest to use DiffValue == 0 then this.
		}
		else if (op == 151) {
			SETOPTION("OnDiffBeginner");
		}
		else if (op == 152) {
			SETOPTION("OnDiffNormal");
		}
		else if (op == 153) {
			SETOPTION("OnDiffHyper");
		}
		else if (op == 154) {
			SETOPTION("OnDiffAnother");
		}
		else if (op == 155) {
			SETOPTION("OnDiffInsane");
		}
		else if (op == 160) {
			SETOPTION("Is7Keys");
		}
		else if (op == 161) {
			SETOPTION("Is5Keys");
		}
		else if (op == 162) {
			SETOPTION("Is14Keys");
		}
		else if (op == 163) {
			SETOPTION("Is10Keys");
		}
		else if (op == 164) {
			SETOPTION("Is9Keys");
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
			SETOPTION("IsP1AAA");
		}
		else if (op == 201) {
			SETOPTION("IsP1AA");
		}
		else if (op == 202) {
			SETOPTION("IsP1A");
		}
		else if (op == 203) {
			SETOPTION("IsP1B");
		}
		else if (op == 204) {
			SETOPTION("IsP1C");
		}
		else if (op == 205) {
			SETOPTION("IsP1D");
		}
		else if (op == 206) {
			SETOPTION("IsP1E");
		}
		else if (op == 207) {
			SETOPTION("IsP1F");
		}
		else if (op == 210) {
			SETOPTION("IsP2AAA");
		}
		else if (op == 211) {
			SETOPTION("IsP2AA");
		}
		else if (op == 212) {
			SETOPTION("IsP2A");
		}
		else if (op == 213) {
			SETOPTION("IsP2B");
		}
		else if (op == 214) {
			SETOPTION("IsP2C");
		}
		else if (op == 215) {
			SETOPTION("IsP2D");
		}
		else if (op == 216) {
			SETOPTION("IsP2E");
		}
		else if (op == 217) {
			SETOPTION("IsP2F");
		}
		else if (op == 220) {
			SETOPTION("IsP1ReachAAA");
		}
		else if (op == 221) {
			SETOPTION("IsP1ReachAA");
		}
		else if (op == 222) {
			SETOPTION("IsP1ReachA");
		}
		else if (op == 223) {
			SETOPTION("IsP1ReachB");
		}
		else if (op == 224) {
			SETOPTION("IsP1ReachC");
		}
		else if (op == 225) {
			SETOPTION("IsP1ReachD");
		}
		else if (op == 226) {
			SETOPTION("IsP1ReachE");
		}
		else if (op == 227) {
			SETOPTION("IsP1ReachF");
		}
		/* 23X : I dont want to implement these useless one ... hmm... don't want ... .... */
		else if (op == 241) {
			SETOPTION("OnP1JudgePerfect");
		}
		else if (op == 242) {
			SETOPTION("OnP1JudgeGreat");
		}
		else if (op == 243) {
			SETOPTION("OnP1JudgeGood");
		}
		else if (op == 244) {
			SETOPTION("OnP1JudgeBad");
		}
		else if (op == 245) {
			SETOPTION("OnP1JudgePoor");
		}
		else if (op == 246) {
			SETOPTION("OnP1JudgePoor");		// ÍöPOOR
		}
		else if (op == 247) {
			SETOPTION("OnP1Miss");
		}
		else if (op == 248) {
			SETNEGATIVEOPTION("OnP1Miss");
		}/*
		else if (op == 220) {
		SETOPTION("IsP1ReachAAA");
		}
		else if (op == 221) {
		SETOPTION("IsP1ReachAA");
		}
		else if (op == 222) {
		SETOPTION("IsP1ReachA");
		}
		else if (op == 223) {
		SETOPTION("IsP1ReachB");
		}
		else if (op == 224) {
		SETOPTION("IsP1ReachC");
		}
		else if (op == 225) {
		SETOPTION("IsP1ReachD");
		}
		else if (op == 226) {
		SETOPTION("IsP1ReachE");
		}
		else if (op == 227) {
		SETOPTION("IsP1ReachF");
		}*/
		/* 23X : I dont want to implement these useless one ... hmm... don't want ... .... */
		else if (op == 261) {
			SETOPTION("OnP2JudgePerfect");
		}
		else if (op == 262) {
			SETOPTION("OnP2JudgeGreat");
		}
		else if (op == 263) {
			SETOPTION("OnP2JudgeGood");
		}
		else if (op == 264) {
			SETOPTION("OnP2JudgeBad");
		}
		else if (op == 265) {
			SETOPTION("OnP2JudgePoor");
		}
		else if (op == 266) {
			SETOPTION("OnP2JudgePoor");		// ÍöPOOR
		}
		else if (op == 267) {
			SETOPTION("OnP2Miss");
		}
		else if (op == 268) {
			SETNEGATIVEOPTION("OnP2Miss");
		}
		/* SUD/LIFT */
		else if (op == 270) {
			SETOPTION("OnP1SuddenChange");
		}
		else if (op == 271) {
			SETOPTION("OnP2SuddenChange");
		}
		/* Course related */
		else if (op == 280) {
			SETOPTION("IsCourse1Stage");
		}
		else if (op == 281) {
			SETOPTION("IsCourse2Stage");
		}
		else if (op == 282) {
			SETOPTION("IsCourse3Stage");
		}
		else if (op == 283) {
			SETOPTION("IsCourse4Stage");
		}
		else if (op == 289) {
			SETOPTION("IsCourseFinal");
		}
		else if (op == 290) {
			SETOPTION("IsCourse");
		}
		else if (op == 291) {
			SETOPTION("IsGrading");		// Ó«êÈìãïÒ
		}
		else if (op == 292) {
			SETOPTION("ExpertCourse");
		}
		else if (op == 293) {
			SETOPTION("ClassCourse");
		}
	}
	/* result screen */
	else if (op >= 300 && op < 400) {
		if (op == 300) {
			SETOPTION("IsP1AAA");
		}
		else if (op == 301) {
			SETOPTION("IsP1AA");
		}
		else if (op == 302) {
			SETOPTION("IsP1A");
		}
		else if (op == 303) {
			SETOPTION("IsP1B");
		}
		else if (op == 304) {
			SETOPTION("IsP1C");
		}
		else if (op == 305) {
			SETOPTION("IsP1D");
		}
		else if (op == 306) {
			SETOPTION("IsP1E");
		}
		else if (op == 307) {
			SETOPTION("IsP1F");
		}
		else if (op == 310) {
			SETOPTION("IsP2AAA");
		}
		else if (op == 311) {
			SETOPTION("IsP2AA");
		}
		else if (op == 312) {
			SETOPTION("IsP2A");
		}
		else if (op == 313) {
			SETOPTION("IsP2B");
		}
		else if (op == 314) {
			SETOPTION("IsP2C");
		}
		else if (op == 315) {
			SETOPTION("IsP2D");
		}
		else if (op == 316) {
			SETOPTION("IsP2E");
		}
		else if (op == 317) {
			SETOPTION("IsP2F");
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

#define SETTIMER(s)\
	strcpy_s(translated, 1024, (s));
const char* _LR2SkinParser::TranslateTimer(int timer) {
	if (timer == 1) {
		SETTIMER("OnStartInput");
	}
	else if (timer == 2) {
		SETTIMER("OnFadeOut");	// FADEOUT
	}
	else if (timer == 3) {
		SETTIMER("OnClose");		// Stage failed
	}
	else if (timer == 21) {
		SETTIMER("OnPanel1");
	}
	else if (timer == 22) {
		SETTIMER("OnPanel2");
	}
	else if (timer == 23) {
		SETTIMER("OnPanel3");
	}
	else if (timer == 24) {
		SETTIMER("OnPanel4");
	}
	else if (timer == 25) {
		SETTIMER("OnPanel5");
	}
	else if (timer == 26) {
		SETTIMER("OnPanel6");
	}
	else if (timer == 27) {
		SETTIMER("OnPanel7");
	}
	else if (timer == 28) {
		SETTIMER("OnPanel8");
	}
	else if (timer == 29) {
		SETTIMER("OnPanel9");
	}
	/* Panel closing: DEPRECIATED */
	else if (timer == 31) {
		SETTIMER("OnPanel1Close");
	}
	else if (timer == 32) {
		SETTIMER("OnPanel2Close");
	}
	else if (timer == 33) {
		SETTIMER("OnPanel3Close");
	}
	else if (timer == 34) {
		SETTIMER("OnPanel4Close");
	}
	else if (timer == 35) {
		SETTIMER("OnPanel5Close");
	}
	else if (timer == 36) {
		SETTIMER("OnPanel6Close");
	}
	else if (timer == 37) {
		SETTIMER("OnPanel7Close");
	}
	else if (timer == 38) {
		SETTIMER("OnPanel8Close");
	}
	else if (timer == 39) {
		SETTIMER("OnPanel9Close");
	}
	else if (timer == 40) {
		SETTIMER("OnReady");
	}
	else if (timer == 41) {
		SETTIMER("OnGameStart");
	}
	else if (timer == 42) {
		SETTIMER("OnP1GaugeUp");
	}
	else if (timer == 43) {
		SETTIMER("OnP2GaugeUp");
	}
	else if (timer == 44) {
		SETTIMER("OnP1GaugeMax");
	}
	else if (timer == 45) {
		SETTIMER("OnP2GaugeMax");
	}
	else if (timer == 46) {
		SETTIMER("OnP1Combo");
	}
	else if (timer == 47) {
		SETTIMER("OnP2Combo");
	}
	else if (timer == 48) {
		SETTIMER("OnP1FullCombo");
	}
	else if (timer == 49) {
		SETTIMER("OnP2FullCombo");
	}
	else if (timer == 50) {
		SETTIMER("OnP1JudgeSCOkay");
	}
	else if (timer == 51) {
		SETTIMER("OnP1Judge1Okay");
	}
	else if (timer == 52) {
		SETTIMER("OnP1Judge2Okay");
	}
	else if (timer == 53) {
		SETTIMER("OnP1Judge3Okay");
	}
	else if (timer == 54) {
		SETTIMER("OnP1Judge4Okay");
	}
	else if (timer == 55) {
		SETTIMER("OnP1Judge5Okay");
	}
	else if (timer == 56) {
		SETTIMER("OnP1Judge6Okay");
	}
	else if (timer == 57) {
		SETTIMER("OnP1Judge7Okay");
	}
	else if (timer == 58) {
		SETTIMER("OnP1Judge8Okay");
	}
	else if (timer == 59) {
		SETTIMER("OnP1Judge9Okay");
	}
	else if (timer == 60) {
		SETTIMER("OnP2JudgeSCOkay");
	}
	else if (timer == 61) {
		SETTIMER("OnP2Judge1Okay");
	}
	else if (timer == 62) {
		SETTIMER("OnP2Judge2Okay");
	}
	else if (timer == 63) {
		SETTIMER("OnP2Judge3Okay");
	}
	else if (timer == 64) {
		SETTIMER("OnP2Judge4Okay");
	}
	else if (timer == 65) {
		SETTIMER("OnP2Judge5Okay");
	}
	else if (timer == 66) {
		SETTIMER("OnP2Judge6Okay");
	}
	else if (timer == 67) {
		SETTIMER("OnP2Judge7Okay");
	}
	else if (timer == 68) {
		SETTIMER("OnP2Judge8Okay");
	}
	else if (timer == 69) {
		SETTIMER("OnP2Judge9Okay");
	}
	else if (timer == 70) {
		SETTIMER("OnP1JudgeSCHold");
	}
	else if (timer == 71) {
		SETTIMER("OnP1Judge1Hold");
	}
	else if (timer == 72) {
		SETTIMER("OnP1Judge2Hold");
	}
	else if (timer == 73) {
		SETTIMER("OnP1Judge3Hold");
	}
	else if (timer == 74) {
		SETTIMER("OnP1Judge4Hold");
	}
	else if (timer == 75) {
		SETTIMER("OnP1Judge5Hold");
	}
	else if (timer == 76) {
		SETTIMER("OnP1Judge6Hold");
	}
	else if (timer == 77) {
		SETTIMER("OnP1Judge7Hold");
	}
	else if (timer == 78) {
		SETTIMER("OnP1Judge8Hold");
	}
	else if (timer == 79) {
		SETTIMER("OnP1Judge9Hold");
	}
	else if (timer == 80) {
		SETTIMER("OnP2JudgeSCHold");
	}
	else if (timer == 81) {
		SETTIMER("OnP2Judge1Hold");
	}
	else if (timer == 82) {
		SETTIMER("OnP2Judge2Hold");
	}
	else if (timer == 83) {
		SETTIMER("OnP2Judge3Hold");
	}
	else if (timer == 84) {
		SETTIMER("OnP2Judge4Hold");
	}
	else if (timer == 85) {
		SETTIMER("OnP2Judge5Hold");
	}
	else if (timer == 86) {
		SETTIMER("OnP2Judge6Hold");
	}
	else if (timer == 87) {
		SETTIMER("OnP2Judge7Hold");
	}
	else if (timer == 88) {
		SETTIMER("OnP2Judge8Hold");
	}
	else if (timer == 89) {
		SETTIMER("OnP2Judge9Hold");
	}
	else if (timer == 100) {
		SETTIMER("OnP1KeySCPress");
	}
	else if (timer == 101) {
		SETTIMER("OnP1Key1Press");
	}
	else if (timer == 102) {
		SETTIMER("OnP1Key2Press");
	}
	else if (timer == 103) {
		SETTIMER("OnP1Key3Press");
	}
	else if (timer == 104) {
		SETTIMER("OnP1Key4Press");
	}
	else if (timer == 105) {
		SETTIMER("OnP1Key5Press");
	}
	else if (timer == 106) {
		SETTIMER("OnP1Key6Press");
	}
	else if (timer == 107) {
		SETTIMER("OnP1Key7Press");
	}
	else if (timer == 108) {
		SETTIMER("OnP1Key8Press");
	}
	else if (timer == 109) {
		SETTIMER("OnP1Key9Press");
	}
	else if (timer == 110) {
		SETTIMER("OnP2KeySCPress");
	}
	else if (timer == 111) {
		SETTIMER("OnP2Key1Press");
	}
	else if (timer == 112) {
		SETTIMER("OnP2Key2Press");
	}
	else if (timer == 113) {
		SETTIMER("OnP2Key3Press");
	}
	else if (timer == 114) {
		SETTIMER("OnP2Key4Press");
	}
	else if (timer == 115) {
		SETTIMER("OnP2Key5Press");
	}
	else if (timer == 116) {
		SETTIMER("OnP2Key6Press");
	}
	else if (timer == 117) {
		SETTIMER("OnP2Key7Press");
	}
	else if (timer == 118) {
		SETTIMER("OnP2Key8Press");
	}
	else if (timer == 119) {
		SETTIMER("OnP2Key9Press");
	}
	else if (timer == 120) {
		SETTIMER("OnP1KeySCUp");
	}
	else if (timer == 121) {
		SETTIMER("OnP1Key1Up");
	}
	else if (timer == 122) {
		SETTIMER("OnP1Key2Up");
	}
	else if (timer == 123) {
		SETTIMER("OnP1Key3Up");
	}
	else if (timer == 124) {
		SETTIMER("OnP1Key4Up");
	}
	else if (timer == 125) {
		SETTIMER("OnP1Key5Up");
	}
	else if (timer == 126) {
		SETTIMER("OnP1Key6Up");
	}
	else if (timer == 127) {
		SETTIMER("OnP1Key7Up");
	}
	else if (timer == 128) {
		SETTIMER("OnP1Key8Up");
	}
	else if (timer == 129) {
		SETTIMER("OnP1Key9Up");
	}
	else if (timer == 130) {
		SETTIMER("OnP2KeySCUp");
	}
	else if (timer == 131) {
		SETTIMER("OnP2Key1Up");
	}
	else if (timer == 132) {
		SETTIMER("OnP2Key2Up");
	}
	else if (timer == 133) {
		SETTIMER("OnP2Key3Up");
	}
	else if (timer == 134) {
		SETTIMER("OnP2Key4Up");
	}
	else if (timer == 135) {
		SETTIMER("OnP2Key5Up");
	}
	else if (timer == 136) {
		SETTIMER("OnP2Key6Up");
	}
	else if (timer == 137) {
		SETTIMER("OnP2Key7Up");
	}
	else if (timer == 138) {
		SETTIMER("OnP2Key8Up");
	}
	else if (timer == 139) {
		SETTIMER("OnP2Key9Up");
	}
	else if (timer == 140) {
		SETTIMER("OnBeat");
	}
	else if (timer == 143) {
		SETTIMER("OnP1LastNote");
	}
	else if (timer == 144) {
		SETTIMER("OnP2LastNote");
	}
	else if (timer == 150) {
		SETTIMER("OnResult");
	}
	else {
		// unknown timer!
		// we can return '0', but I think it's better to return "UNKNOWN", for safety.
		return 0;
		//return "UNKNOWN";
	}
	return translated;
}

// TODO: quite things to consider
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

#define SETSLIDER(s)\
	strcpy_s(translated, 1024, (s))
const char* _LR2SkinParser::TranslateSlider(int code) {
	if (code == 1) {
		SETSLIDER("SelectBar");
	}
	else if (code == 2) {
		SETSLIDER("P1HighSpeed");
	}
	else if (code == 3) {
		SETSLIDER("P2HighSpeed");
	}
	else if (code == 4) {
		SETSLIDER("P1Sudden");
	}
	else if (code == 5) {
		SETSLIDER("P2Sudden");
	}
	else if (code == 6) {
		SETSLIDER("PlayProgress");
	}
	else if (code == 17) {
		SETSLIDER("Volume");
	}
	else if (code == 26) {
		SETSLIDER("Pitch");
	}
	// Lift isn't supported in LR2
	/* else (skin scroll, FX, etc ...) are all depreciated, so ignore. */
	else {
		return 0;
	}

	return translated;
}

#define SETGRAPH(s)\
	strcpy_s(translated, 1024, (s))
const char* _LR2SkinParser::TranslateGraph(int code) {
	if (code == 1) {
		SETGRAPH("PlayProgress");			// shares with number
	}
	else if (code == 2) {
		SETGRAPH("SongLoadProgress");
	}
	else if (code == 3) {
		SETGRAPH("SongLoadProgress");
	}
	else if (code == 5) {
		SETGRAPH("BeginnerLevel");	// graph only value
	}
	else if (code == 6) {
		SETGRAPH("NormalLevel");
	}
	else if (code == 7) {
		SETGRAPH("HyperLevel");
	}
	else if (code == 8) {
		SETGRAPH("AnotherLevel");
	}
	else if (code == 9) {
		SETGRAPH("InsaneLevel");
	}
	else if (code == 10) {
		SETGRAPH("P1ExScore");
	}
	else if (code == 11) {
		SETGRAPH("P1ExScoreEsti");
	}
	else if (code == 12) {
		SETGRAPH("P1HighScore");
	}
	else if (code == 13) {
		SETGRAPH("P1HighScoreEsti");
	}
	else if (code == 14) {
		SETGRAPH("P2ExScore");
	}
	else if (code == 15) {
		SETGRAPH("P2ExScoreEsti");
	}
	else if (code == 20) {
		SETGRAPH("ResultPerfectPercent");
	}
	else if (code == 21) {
		SETGRAPH("ResultGreatPercent");
	}
	else if (code == 22) {
		SETGRAPH("ResultGoodPercent");
	}
	else if (code == 23) {
		SETGRAPH("ResultBadPercent");
	}
	else if (code == 24) {
		SETGRAPH("ResultPoorPercent");
	}
	else if (code == 25) {
		SETGRAPH("ResultMaxComboPercent");
	}
	else if (code == 26) {
		SETGRAPH("ResultScorePercent");
	}
	else if (code == 27) {
		SETGRAPH("ResultExScorePercent");
	}
	else if (code == 30) {
		SETGRAPH("GhostPerfectPercent");
	}
	else if (code == 31) {
		SETGRAPH("GhostGreatPercent");
	}
	else if (code == 32) {
		SETGRAPH("GhostGoodPercent");
	}
	else if (code == 33) {
		SETGRAPH("GhostBadPercent");
	}
	else if (code == 34) {
		SETGRAPH("GhostPoorPercent");
	}
	else if (code == 35) {
		SETGRAPH("GhostMaxComboPercent");
	}
	else if (code == 36) {
		SETGRAPH("GhostScorePercent");
	}
	else if (code == 37) {
		SETGRAPH("GhostExScorePercent");
	}
	// 40 ~ 47 highscore is depreciated; ignore
	else {
		return 0;
	}

	return translated;
}

#define SETNUMBER(s)\
	strcpy_s(translated, 1024, (s))
const char* _LR2SkinParser::TranslateNumber(int code) {
	if (code == 10) {
		SETNUMBER("P1Speed");
	}
	else if (code == 11) {
		SETNUMBER("P2Speed");
	}
	else if (code == 12) {
		SETNUMBER("JudgeTiming");
	}
	else if (code == 13) {
		SETNUMBER("TargetRate");
	}
	else if (code == 14) {
		SETNUMBER("P1Sudden");
	}
	else if (code == 15) {
		SETNUMBER("P2Sudden");
	}/* LR2 doesn't support lift option
	else if (code == 14) {
		SETNUMBER("P2Lift");
	}
	else if (code == 15) {
		SETNUMBER("P2Lift");
	}*/
	else if (code == 20) {
		SETNUMBER("FPS");
	}
	else if (code == 21) {
		SETNUMBER("Year");
	}
	else if (code == 22) {
		SETNUMBER("Month");
	}
	else if (code == 23) {
		SETNUMBER("Day");
	}
	else if (code == 24) {
		SETNUMBER("Hour");
	}
	else if (code == 25) {
		SETNUMBER("Minute");
	}
	else if (code == 26) {
		SETNUMBER("Second");
	}
	else if (code == 30) {
		SETNUMBER("TotalPlayCount");
	}
	else if (code == 31) {
		SETNUMBER("TotalClearCount");
	}
	else if (code == 32) {
		SETNUMBER("TotalFailCount");
	}
	else if (code == 45) {
		SETNUMBER("BeginnerLevel");
	}
	else if (code == 46) {
		SETNUMBER("NormalLevel");
	}
	else if (code == 47) {
		SETNUMBER("HyperLevel");
	}
	else if (code == 48) {
		SETNUMBER("AnotherLevel");
	}
	else if (code == 49) {
		SETNUMBER("InsaneLevel");
	}
	else if (code == 70) {
		SETNUMBER("Score");
	}
	else if (code == 71) {
		SETNUMBER("ExScore");
	}
	else if (code == 72) {
		SETNUMBER("ExScore");
	}
	else if (code == 73) {
		SETNUMBER("Rate");
	}
	else if (code == 74) {
		SETNUMBER("TotalNotes");
	}
	else if (code == 75) {
		SETNUMBER("MaxCombo");
	}
	else if (code == 76) {
		SETNUMBER("MinBP");
	}
	else if (code == 77) {
		SETNUMBER("PlayCount");
	}
	else if (code == 78) {
		SETNUMBER("ClearCount");
	}
	else if (code == 79) {
		SETNUMBER("FailCount");
	}
	else if (code == 80) {
		SETNUMBER("PerfectCount");
	}
	else if (code == 81) {
		SETNUMBER("GreatCount");
	}
	else if (code == 82) {
		SETNUMBER("GoodCount");
	}
	else if (code == 83) {
		SETNUMBER("BadCount");
	}
	else if (code == 84) {
		SETNUMBER("PoorCount");
	}
	else if (code == 90) {
		SETNUMBER("BPMMax");
	}
	else if (code == 91) {
		SETNUMBER("BPMMin");
	}
	else if (code == 92) {
		SETNUMBER("IRRank");
	}
	else if (code == 93) {
		SETNUMBER("IRTotal");
	}
	else if (code == 94) {
		SETNUMBER("IRRate");
	}
	else if (code == 95) {
		SETNUMBER("RivalDiff");		// abs(HighExScore - HighExScoreRival)
	}
	/* during play */
	else if (code == 100) {
		SETNUMBER("P1Score");
	}
	else if (code == 101) {
		SETNUMBER("P1ExScore");
	}
	else if (code == 102) {
		SETNUMBER("P1Rate");
	}
	else if (code == 103) {
		SETNUMBER("P1Rate_decimal");
	}
	else if (code == 104) {
		SETNUMBER("P1Combo");
	}
	else if (code == 105) {
		SETNUMBER("P1MaxCombo");
	}
	else if (code == 106) {
		SETNUMBER("P1TotalNotes");
	}
	else if (code == 107) {
		SETNUMBER("P1Gauge");
	}
	else if (code == 108) {
		SETNUMBER("P1RivalDiff");
	}
	else if (code == 110) {
		SETNUMBER("P1PerfectCount");
	}
	else if (code == 111) {
		SETNUMBER("P1GreatCount");
	}
	else if (code == 112) {
		SETNUMBER("P1GoodCount");
	}
	else if (code == 113) {
		SETNUMBER("P1BadCount");
	}
	else if (code == 114) {
		SETNUMBER("P1PoorCount");
	}
	else if (code == 115) {
		SETNUMBER("P1TotalRate");			// estimated value
	}
	else if (code == 116) {
		SETNUMBER("P1TotalRate_decimal");	// TODO: process with Lua code
	}
	/* ghost */
	else if (code == 120) {
		SETNUMBER("P2Score");
	}
	else if (code == 121) {
		SETNUMBER("P2ExScore");
	}
	else if (code == 122) {
		SETNUMBER("P2Rate");
	}
	else if (code == 123) {
		SETNUMBER("P2Rate_decimal");
	}
	else if (code == 124) {
		SETNUMBER("P2Combo");
	}
	else if (code == 125) {
		SETNUMBER("P2MaxCombo");
	}
	else if (code == 126) {
		SETNUMBER("P2TotalNotes");
	}
	else if (code == 127) {
		SETNUMBER("P2Gauge");
	}
	else if (code == 128) {
		SETNUMBER("P2RivalDiff");
	}
	else if (code == 130) {
		SETNUMBER("P2PerfectCount");
	}
	else if (code == 131) {
		SETNUMBER("P2GreatCount");
	}
	else if (code == 132) {
		SETNUMBER("P2GoodCount");
	}
	else if (code == 133) {
		SETNUMBER("P2BadCount");
	}
	else if (code == 134) {
		SETNUMBER("P2PoorCount");
	}
	else if (code == 135) {
		SETNUMBER("P2TotalRate");	// estimated value
	}
	else if (code == 136) {
		SETNUMBER("P2TotalRate_decimal");
	}
	/*
	 * 150 ~ 158: TODO (useless?)
	 */
	else if (code == 160) {
		SETNUMBER("PlayBPM");
	}
	else if (code == 161) {
		SETNUMBER("PlayMinute");
	}
	else if (code == 162) {
		SETNUMBER("PlaySecond");
	}
	else if (code == 163) {
		SETNUMBER("PlayRemainMinute");
	}
	else if (code == 164) {
		SETNUMBER("PlayRemainSecond");
	}
	else if (code == 165) {
		SETNUMBER("PlayProgress");	// (%)
	}
	else if (code == 170) {
		SETNUMBER("ResultExScoreBefore");
	}
	else if (code == 171) {
		SETNUMBER("ResultExScoreNow");
	}
	else if (code == 172) {
		SETNUMBER("ResultExScoreDiff");
	}
	else if (code == 173) {
		SETNUMBER("ResultMaxComboBefore");
	}
	else if (code == 174) {
		SETNUMBER("ResultMaxComboNow");
	}
	else if (code == 175) {
		SETNUMBER("ResultMaxComboDiff");
	}
	else if (code == 176) {
		SETNUMBER("ResultMinBPBefore");
	}
	else if (code == 177) {
		SETNUMBER("ResultMinBPNow");
	}
	else if (code == 178) {
		SETNUMBER("ResultMinBPDiff");
	}
	else if (code == 179) {
		SETNUMBER("ResultIRRankNow");
	}
	else if (code == 180) {
		SETNUMBER("ResultIRRankTotal");
	}
	else if (code == 181) {
		SETNUMBER("ResultIRRankRate");
	}
	else if (code == 182) {
		SETNUMBER("ResultIRRankBefore");
	}
	else if (code == 183) {
		SETNUMBER("ResultRate");
	}
	else if (code == 184) {
		SETNUMBER("ResultRate_decimal");
	}
	/* ignore IR Beta3: 200 ~ 250 */
	/* rival (in select menu) */
	else if (code == 270) {
		SETNUMBER("RivalScore");
	}
	else if (code == 271) {
		SETNUMBER("RivalExScore");
	}
	else if (code == 272) {
		SETNUMBER("RivalRate");
	}
	else if (code == 273) {
		SETNUMBER("RivalRate_decimal");
	}
	else if (code == 274) {
		SETNUMBER("RivalCombo");
	}
	else if (code == 275) {
		SETNUMBER("RivalMaxCombo");
	}
	else if (code == 276) {
		SETNUMBER("RivalTotalNotes");
	}
	else if (code == 277) {
		SETNUMBER("RivalGrooveGauge");
	}
	else if (code == 278) {
		SETNUMBER("RivalRivalDiff");
	}
	else if (code == 280) {
		SETNUMBER("RivalPerfectCount");
	}
	else if (code == 281) {
		SETNUMBER("RivalGreatCount");
	}
	else if (code == 282) {
		SETNUMBER("RivalGoodCount");
	}
	else if (code == 283) {
		SETNUMBER("RivalBadCount");
	}
	else if (code == 284) {
		SETNUMBER("RivalPoorCount");
	}
	/* 285 ~ is depreciated. ignore. */
	else {
		return 0;
	}

	return translated;
}

#define SETTEXT_(s)\
	strcpy_s(translated, 1024, (s))
const char* _LR2SkinParser::TranslateText(int code) {
	if (code == 1) {
		SETTEXT_("RivalName"); 
	}
	else if (code == 2) {
		SETTEXT_("PlayerName");
	}
	else if (code == 10) {
		SETTEXT_("Title");
	}
	else if (code == 11) {
		SETTEXT_("Subtitle");
	}
	else if (code == 12) {
		SETTEXT_("MainTitle");
	}
	else if (code == 13) {
		SETTEXT_("Genre");
	}
	else if (code == 14) {
		SETTEXT_("Artist");
	}
	else if (code == 15) {
		SETTEXT_("SubArtist");
	}
	else if (code == 16) {
		SETTEXT_("SearchTag");
	}
	else if (code == 17) {
		SETTEXT_("PlayLevel");		// depreciated?
	}
	else if (code == 18) {
		SETTEXT_("PlayDiff");			// depreciated?
	}
	else if (code == 19) {
		SETTEXT_("PlayInsaneLevel");	// depreciated?
	}
	/*
	 * 20 ~ 30: for editing (depreciated/ignore?)
	 */
	else if (code == 40) {
		SETTEXT_("KeySlot0");
	}
	else if (code == 41) {
		SETTEXT_("KeySlot1");
	}
	else if (code == 42) {
		SETTEXT_("KeySlot2");
	}
	else if (code == 43) {
		SETTEXT_("KeySlot3");
	}
	else if (code == 44) {
		SETTEXT_("KeySlot4");
	}
	else if (code == 45) {
		SETTEXT_("KeySlot5");
	}
	else if (code == 46) {
		SETTEXT_("KeySlot6");
	}
	else if (code == 47) {
		SETTEXT_("KeySlot7");
	}
	/* Skin select window */
	else if (code == 50) {
		SETTEXT_("SkinName");
	}
	else if (code == 51) {
		SETTEXT_("SkinAuthor");
	}
	/* option */
	else if (code == 60) {
		SETTEXT_("PlayMode");
	}
	else if (code == 61) {
		SETTEXT_("PlaySort");
	}
	else if (code == 62) {
		SETTEXT_("PlayDiff");
	}
	else if (code == 63) {
		SETTEXT_("RandomP1");
	}
	else if (code == 64) {
		SETTEXT_("RandomP2");
	}
	else if (code == 65) {
		SETTEXT_("GaugeP1");
	}
	else if (code == 66) {
		SETTEXT_("GaugeP2");
	}
	else if (code == 67) {
		SETTEXT_("AssistP1");
	}
	else if (code == 68) {
		SETTEXT_("AssistP2");
	}
	else if (code == 69) {
		SETTEXT_("Battle");		// depreciated?
	}
	else if (code == 70) {
		SETTEXT_("Flip");			// depreciated?
	}
	else if (code == 71) {
		SETTEXT_("ScoreGraph");	// depreciated?
	}
	else if (code == 72) {
		SETTEXT_("Ghost");
	}
	else if (code == 74) {
		SETTEXT_("ScrollType");
	}
	else if (code == 75) {
		SETTEXT_("BGASize");		// depreciated
	}
	else if (code == 76) {
		SETTEXT_("IsBGA");		// depreciated?
	}/*
	screen color: depreciated
	else if (code == 60) {
		SETTEXT_("ScreenColor");
	}*/
	else if (code == 78) {
		SETTEXT_("VSync");
	}
	else if (code == 79) {
		SETTEXT_("ScreenMode");	// full/window
	}
	else if (code == 80) {
		SETTEXT_("AutoJudge");
	}
	else if (code == 81) {
		SETTEXT_("ReplaySave");
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
	lines_.clear();
	line_args_.clear();
	memset(condition_status, 0, sizeof(condition_status));
}

// ----------------------- LR2Skin part end ------------------------
