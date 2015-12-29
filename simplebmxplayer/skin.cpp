#include "skin.h"
#include "game.h"

/*
 * SkinDST
 * - global accessible variable
 */
namespace SkinDST {
	bool dst[10000];
	void On(int idx) {
		dst[idx] = true;
	}
	void Off(int idx) {
		dst[idx] = false;
	}
	bool Toggle(int idx) {
		dst[idx] = !dst[idx];
		return dst[idx];
	}
	bool Get(int idx) {
		return dst[idx];
	}
}

// temporarily common function
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

bool Skin::Parse(const char *filepath) {
	//strcpy(skin->filepath, filepath);
	FILE *f = fopen(filepath, "r");
	if (!f)
		return false;

	parser_condition[parser_level] = true;

	char line[1024];
	char *p;
	char *args[30];
	while (!feof(f)) {
		fgets(line, 1024, f);
		p = Trim(line);
		args[0] = p;
		for (int i = 1; i < 30; i++) {
			p = strchr(p, ',');
			if (!p)
				break;
			*p = 0;
			args[i] = (++p);
		}
		MakeElementFromLr2Skin(args);
	}

	fclose(f);

	if (parser_level == 0)
		LoadResource();

	return true;
}

void Skin::MakeElementFromLr2Skin(char **args) {
	if (elements.size() == 0)
		elements.push_back(SkinElement());
	SkinElement *e = &elements.back();
#define CMD_IS(v) (strcmp(args[0], (v)) == 0)
#define CMD_STARTSWITH(v,s) (strncmp(args[0], (v), (s)) == 0)
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
		parser_level--;
	}
	if (parser_condition[parser_level] < 0)	// processed if clause -> IGNORE ALL
		return;
	else if (CMD_IS("#ELSEIF")) {
		if (parser_condition[parser_level] == 1) {
			parser_condition[parser_level] = -1;
			return;
		}
		parser_condition[parser_level] = SkinDST::Get(INT(args[1]));
	}
	else if (CMD_IS("#ELSE")) {
		if (parser_condition[parser_level] == 1) {
			parser_condition[parser_level] = -1;
			return;
		}
		parser_condition[parser_level] = !parser_condition[parser_level];
	}
	else if (CMD_IS("#IF")) {
		parser_condition[++parser_level] = SkinDST::Get(INT(args[1]))?1:0;
	}
	if (parser_condition[parser_level] == 0)
		return;

	/*
	 * object parsing start
	 */
	if (CMD_IS("#CUSTOMOPTION")) {	// use default value
		SkinDST::On(atoi(args[2]));
	}
	else if (CMD_IS("#INCLUDE")) {
		parser_level++;
		Parse(args[1]);
		parser_level--;
	}
	else if (CMD_IS("#IMAGE")) {
		auto it = headers.find(args[0] + 1);
		if (it == headers.end()) {
			std::vector<std::string> v;
			headers.insert(
				std::map<std::string, std::vector<std::string>>::value_type(args[0] + 1, v));
		}
		std::vector<std::string> &v = headers["IMAGE"];
		v.push_back(args[1]);
	}
	else if (CMD_IS("#FONT")) {

	}
	else {
		// note part is a little problematic ...
		if (CMD_IS("#SRC_NOTE")) {
			note[INT(args[1])].AddSrc(args + 1, imgs);
		}
		else if (CMD_IS("#DST_NOTE")) {
			note[INT(args[1])].AddDst(args + 1);
			lnstart[INT(args[1])].AddDst(args + 1);
			lnbody[INT(args[1])].AddDst(args + 1);
			lnend[INT(args[1])].AddDst(args + 1);
		}
		else if (CMD_IS("#SRC_LN_START")) {
			lnstart[INT(args[1])].AddSrc(args + 1, imgs);
		}
		else if (CMD_IS("#DST_LN_START")) {
			lnstart[INT(args[1])].AddDst(args + 1);
		}
		else if (CMD_IS("#SRC_LN_BODY")) {
			lnbody[INT(args[1])].AddSrc(args + 1, imgs);
		}
		else if (CMD_IS("#DST_LN_BODY")) {
			lnbody[INT(args[1])].AddDst(args + 1);
		}
		else if (CMD_IS("#SRC_LN_END")) {
			lnend[INT(args[1])].AddSrc(args + 1, imgs);
		}
		else if (CMD_IS("#DST_LN_END")) {
			lnend[INT(args[1])].AddDst(args + 1);
		}
		// bargraph also ...
		else if (CMD_IS("#SRC_BARGRAPH")) {

		}
		else if (CMD_IS("#DST_BARGRAPH")) {

		}
		// slider also ...
		else if (CMD_IS("#SRC_SLIDER")) {

		}
		else if (CMD_IS("#DST_SLIDER")) {

		}
		// BGA
		else if (CMD_IS("#DST_BGA")) {
			bga.x = INT(args[3]);
			bga.y = INT(args[4]);
			bga.w = INT(args[5]);
			bga.h = INT(args[6]);
		}
		//
		else if (CMD_IS("#SRC_IMAGE")) {
			// should we add new element?
			if (e->IsValid()) {
				elements.push_back(SkinElement());
				e = &elements.back();
			}
			e->AddSrc(args + 1, imgs);
		}
		else if (CMD_IS("#DST_IMAGE")) {
			e->AddDst(args + 1);
		}
	}
#undef CMD_IS
}

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

void SkinElement::AddSrc(char **args, Image *imgs) {
	ImageSRC src;
	if (this->src.size() == 0) {
		img = &imgs[INT(args[1])];
	}
	src.x = INT(args[2]);
	src.y = INT(args[3]);
	src.w = INT(args[4]);
	src.h = INT(args[5]);
	src.div_x = INT(args[6]);
	src.div_y = INT(args[7]);
	src.cycle = INT(args[8]);
	src.timer = INT(args[9]);
	AddSrc(src);
}

void SkinElement::AddDst(char **args) {
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

		option[0] = INT(args[17]);
		option[1] = INT(args[18]);
		option[2] = INT(args[19]);
	}
	AddDst(dst);
}

void SkinElement::AddSrc(ImageSRC &src) {
	this->src.push_back(src);
}

void SkinElement::AddDst(ImageDST &dst) {
	this->dst.push_back(dst);
}

bool SkinElement::IsValid() {
	return (src.size() && dst.size());
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

void SkinElement::GetRenderData(SkinRenderData &renderdata) {
	// since it's not completed, we'll provide temp data ...
	renderdata.src = this->src.back();
	renderdata.dst = this->dst.back();
	renderdata.blend = this->blend;
	if (timer == 0 && CheckOption())
		renderdata.img = this->img;
	else
		renderdata.img = 0;
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