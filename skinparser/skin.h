// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "tinyxml2.h"
#include "skinrendertree.h"
#include <vector>
#include <map>

using namespace tinyxml2;

/*
 * Skin infos that we're using
 */
struct SkinFont {
	std::string filepath;		// ttf filepath
	std::string texture;		// foreground texture path
	int fontsize;
	int thickness;
	int style;
	int border;
};

class Skin {
public:
	// very basic metadata
	char filepath[256];

	// skin layout data is here
	XMLDocument skinlayout;

	// render tree used for rendering
	SkinRenderTree skinbody;
private:
	void CreateRenderTree();
public:
	Skin();
	~Skin();
	void Release();
	bool Parse(const char *filepath);
	bool Save(const char *filepath);
};

/*
* SkinOption
* stores previously setted option value / provides value from option
*/
class SkinOption {
public:
	struct FileOption {
		std::string path;
		int type;			// 0: image, 1: font
	};

private:
	// the real stored value in option file
	std::map<std::string, int> option_string_idx;
	std::map<std::string, FileOption> option_file_idx;

public:
	// used when modify/add option
	void ModifyOption(const std::string& key, int val);
	void ModifyFileOption(const std::string& key, const std::string& path);
	void SetOptionType(const std::string& key, int type);

	//
	int GetOption(const std::string& key);
	std::string& GetFileOption(const std::string& key);
	// this method surely provides existing file so don't worry.
	bool IsOptionKeyExists(const std::string& key);
	bool IsFileOptionKeyExists(const std::string& key);

	void DefaultSkinOption(Skin &s);	// TODO
	void LoadSkinOption(Skin &s);
	void SaveSkinOption();

	void Clear();
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
	 * ONLY for LR2 SKIN parsing (*depreciated*)
	 */
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
	// pacemaker: use default XML
public:
	// after parsing, this will automatically call Clear();
	bool ParseLR2Skin(const char *filepath, Skin *s);
	void Clear();
};