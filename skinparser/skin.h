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

class _LR2SkinParser {
private:
	Skin *s;
	XMLElement *cur_e;					// current base element
	char lines[10240][10240];			// contain included lines
	char *line_args[10240][100];		// contain arguments for each lines
	int line_position[10240];			// line position per each included files
	int line_total;						// the line we totally read

	/*
	 * 
	 */
	int image_cnt;												// only for setting image name
	int font_cnt;												// only for setting font name
	std::map<std::string, std::string> filter_to_optionname;	// convert filter to optionfile name
private:
	/*
	 * Should support nested condition, at least
	 */
	XMLElement *condition_element[100];
	int condition_level;

	/*
	 * OP/Timer code translator
	 */
	char translated[1024];
	const char* TranslateOPs(int op);		// LR2 op code to useful visible tag
	const char* TranslateTimer(int timer);	// 

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
	// after parsing, this will automatically call Clear();
	bool ParseLR2Skin(const char *filepath, Skin *s);
	void Clear();
};