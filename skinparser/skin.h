// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "skinoption.h"
#include "tinyxml2.h"
#include <vector>
#include <map>

using namespace tinyxml2;

class Skin {
public:
	// very basic metadata
	char filepath[256];

	// skin layout data is here
	XMLDocument skinlayout;

	// render tree used for rendering
	//SkinRenderTree skinbody;
public:
	Skin();
	~Skin();
	void Release();
	bool Parse(const char *filepath);
	bool Save(const char *filepath);

	// must call after skin data is loaded
	void GetDefaultOption(SkinOption *o);
};


// -------------------------------------------------------------
#define MAX_LINE 20000
#define MAX_LINE_CHARACTER 1024

class _LR2SkinParser {
private:
	Skin *s;
	XMLElement *cur_e;							// current base element
	char lines[MAX_LINE][MAX_LINE_CHARACTER];	// contain included lines
	char *line_args[MAX_LINE][100];				// contain arguments for each lines
	int line_position[MAX_LINE];				// line position per each included files
	int line_total;								// the line we totally read

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
	XMLElement *condition_element[100];
	int condition_level;

	/*
	 * OP/Timer/etc code translator
	 */
	int GenerateTexturefontString(XMLElement *srcelement, int startgidx, int endgidx, bool minus = false);	// for #XXX_NUMBER object
	void ConvertToTextureFont(XMLElement *numele);

	/*
	 * Inner skin loaders
	 */
	int LoadSkin(const char *filepath, int linebufferpos = 0);
	void ParseSkin();
	int ParseSkinLine(int line);			// returns next line to be parsed
	void ParseSkinLineArgument(char *line, char** args);
private:
	/*
	 * under are a little macros
	 */
	int ProcessLane(XMLElement *src, int line);			// process commands about lane
	int ProcessCombo(XMLElement *obj, int line);		// process commands about combo
	int ProcessSelectBar(XMLElement *obj, int line);	// process commands about select bar
	int ProcessSelectBar_DST(int line);					// process commands about select bar
	// pacemaker: use default XML
public:
	static const char* TranslateOPs(int op);						// LR2 op code to useful visible tag (timer)
	static const char* TranslateTimer(int timer);					// translate code (timer/handler)
	static const char* TranslateSlider(int code);					// slider to value code (float/int/string)
	static const char* TranslateGraph(int code);					// BARGRAPH to value code (float/int/string)
	static const char* TranslateNumber(int code);					// number to value code (float/int/string)
	static const char* TranslateText(int code);						// text to value code (float/int/string)

	// after parsing, this will automatically call Clear();
	bool ParseLR2Skin(const char *filepath, Skin *s);
	void Clear();
};