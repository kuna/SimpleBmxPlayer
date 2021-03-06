// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "skinoption.h"
#include "tinyxml2.h"
#include <vector>
#include <map>

class Skin {
public:
	// very basic metadata
	char filepath[256];

	// skin layout data is here
	tinyxml2::XMLDocument skinlayout;

	// render tree used for rendering
	//SkinRenderTree skinbody;
public:
	Skin();
	~Skin();
	void Release();
	bool Load(const char *filepath);
	bool Save(const char *filepath);

	// must call after skin data is loaded
	void GetDefaultOption(SkinOption *o);
};


// -------------------------------------------------------------
#define MAX_LINE_CHARACTER_	1024
#define MAX_ARGS_COUNT_		100

class _LR2SkinParser {
private:
	typedef char line_[MAX_LINE_CHARACTER_];
	typedef const char* args_[MAX_ARGS_COUNT_];
	typedef const char** args_read_;
	struct line_v_ { line_ line__; };
	struct args_v_ { args_ args__; };

private:
	Skin *s;
	tinyxml2::XMLElement *cur_e;				// current base element
	std::vector<line_v_> lines_;					// contains line data
	std::vector<args_v_> line_args_;				// contains arguments per line
	int line_total;								// the line we totally read
	char filepath[1024];						// current file name

	/*
	 * 
	 */
	int image_cnt;												// only for setting image name
	int font_cnt;												// only for setting font name
	std::map<std::string, std::string> filter_to_optionname;	// convert filter to optionfile name
	std::map<int, int> texturefont_id;							// check texturefont existence
private:
	/*
	 * Should support nested condition, at least
	 */
	tinyxml2::XMLElement *condition_element[100];
	int condition_status[100];		// access count for that level. only for #IMAGE parsing.
	int condition_level;

	/*
	 * OP/Timer/etc code translator
	 */
	int GenerateTexturefontString(tinyxml2::XMLElement *srcelement);	// for #XXX_NUMBER object
	void ConvertToTextureFont(tinyxml2::XMLElement *numele);

	/*
	 * Inner skin loaders
	 */
	int LoadSkin(const char *filepath, int linebufferpos = 0);
	void ParseSkin();
	int ParseSkinLine(int line);			// returns next line to be parsed
	void ParseSkinLineArgument(char *line, const char** args);
private:
	/*
	 * under are a little macros
	 */
	void ProcessNumber(tinyxml2::XMLElement* obj, int sop1, int sop2, int sop3);
	int ProcessLane(tinyxml2::XMLElement *src, int line, int resid);			// process commands about lane
	int ProcessCombo(tinyxml2::XMLElement *obj, int line);		// process commands about combo
	int ProcessSelectBar(tinyxml2::XMLElement *obj, int line);	// process commands about select bar
	int ProcessSelectBar_DST(int line);					// process commands about select bar
	// pacemaker: use default XML
private:
	/*
	 * there're 3 pools: string/number(float)/timer(handler)
	 * and we can call these pool by string name. 
	 * and these function converts LR2 name into string name.
	 */
	static const char* TranslateOPs(int op);			// LR2 op code to useful visible tag (timer)
	static const char* TranslateTimer(int timer);		// translate code (timer/handler)
	static const char* TranslateButton(int code);		// translate code to handler (timer/handler)
	static const char* TranslateSlider(int code);		// slider to value code (float)
	static const char* TranslateGraph(int code);		// BARGRAPH to value code (float)
	static const char* TranslateNumber(int code);		// number to value code (float)
	static const char* TranslateText(int code);			// text to value code (string)

public:
	// for extern use
	void AddPathToOption(const std::string& path, const std::string& option) {
		filter_to_optionname.insert(std::pair<std::string, std::string>(path, option));
	};

	// after parsing, this will automatically call Clear();
	bool ParseLR2Skin(const char *filepath, Skin *s);
	void Clear();

	_LR2SkinParser() { Clear(); }
};