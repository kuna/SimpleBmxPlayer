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
			if (e->src.size() == 0) {
				e->CopyOptions(parser_condition);
				e->objname = args[0] + 4;
			}
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
	SkinElement* newobj = new SkinElement(true, debugInfo);
	bool buildnewobj = true;

	// fill information
	// TODO ... add metadata
	newobj->GetSrcArray() = e->src;
	newobj->GetDstArray() = e->dst;
	newobj->SetTag(e->objname);

	// make classname from ops/timer
	if (e->CheckOption(32)) {
		newobj->AddClassName("AutoPlay");
	}
	if (e->CheckOption(33)) {
		newobj->AddClassName("AutoPlayOff");
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
	if (e->CheckOption(50)) {
		newobj->AddClassName("Offline");
	}
	if (e->CheckOption(51)) {
		newobj->AddClassName("Online");
	}

	if (e->timer == 40) {
		newobj->AddClassName("OnReady");
	}
	else if (e->timer == 41) {
		newobj->AddClassName("OnGameStart");
	}
	else if (e->timer == 46) {
		newobj->AddClassName("On1PJudge");
	}
	else if (e->timer == 47) {
		newobj->AddClassName("On2PJudge");
	}

	// find some specific tags
	if (e->objname == "IMAGE") {
		if (e->resource_id == 110) {
			// I don't know what it is, it's called bga mask ...
			// but is it necessary ..?
			buildnewobj = false;
		}
	}
	else if (e->objname == "BGA") {
		// we shouldn't set ID here because many BGA can be existed.
		newobj->AddClassName("BGA");
	}
	else if (e->objname == "BARGRAPH") {
		if (e->muki == 1)
			newobj->SetTag("BARGRAPH_VERTICAL");
		else
			newobj->SetTag("BARGRAPH_HORIZON");

		if (e->value_id == 12)
			newobj->SetID("1PCurrentHighScoreGraph");
		if (e->value_id == 13)
			newobj->SetID("1PHighScoreGraph");
		if (e->value_id == 13)
			newobj->SetID("1PCurrentExScoreGraph");
		if (e->value_id == 15)
			newobj->SetID("1PExScoreGraph");
	}
	else if (e->objname == "NUMBER")
	{
		if (e->value_id == 101)
			newobj->SetID("1PExScore");
		if (e->value_id == 151)
			newobj->SetID("1PCurrentHighScore");
		if (e->value_id == 151)
			newobj->SetID("1PCurrentTargetScore");
	}

	if (buildnewobj) {
		return newobj;
	}
	else {
		delete newobj;
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
	ImageSRC src;
	if (this->src.size() == 0) {
		resource_id = INT(args[1]);
		timer_src = INT(args[9]);
		divx = INT(args[6]);
		divy = INT(args[7]);
		looptime_src = INT(args[8]);
	}
	src.x = INT(args[2]);
	src.y = INT(args[3]);
	src.w = INT(args[4]);
	src.h = INT(args[5]);
	src.div_x = INT(args[6]);
	src.div_y = INT(args[7]);
	this->src.push_back(src);
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
	return (src.size() && dst.size());
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
	src.clear();
	dst.clear();
	options.clear();
}

// ----------------------- LR2Skin part end ------------------------

void Skin::LoadResource() {
	// search image part
	std::vector<std::string> &imgs_header = headers["IMAGE"];
	int i = 0;
	for (auto it = imgs_header.begin(); it != imgs_header.end(); ++it) {
		if (!imgs[i].Load((*it)))
			printf("Failed to load skin resource - %s\n", (*it).c_str());
		i++;
	}
}

void Skin::Release() {
	for (int i = 0; i < 20; i++) {
		imgs[i].Release();
	}
	elements.clear();
	headers.clear();
}

Skin::Skin() {}
Skin::~Skin() { Release(); }

std::vector<SkinElement>::iterator Skin::begin() {
	return elements.begin();
}

std::vector<SkinElement>::iterator Skin::end() {
	return elements.end();
}

// -------------------------------------------------------


void SkinElement::AddSrc(ImageSRC &src) {
	this->src.push_back(src);
}

void SkinElement::AddDst(ImageDST &dst) {
	this->dst.push_back(dst);
}

bool SkinElement::CheckOption() {
	bool r = true;
	for (int i = 0; i < 3 && r; i++) {
		if (option[i] < 0) {
			r = !SkinDST::Get(option[i]);
		} 
		else if (option[i] > 0) {
			r = SkinDST::Get(option[i]);
		}
	}
	return r;
}

// --------------------------------------------------------

void ImageSRC::ToRect(SDL_Rect &r) {
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
}

void ImageDST::ToRect(SDL_Rect &r) {
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
}