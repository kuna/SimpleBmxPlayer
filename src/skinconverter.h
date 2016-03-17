/*
 * @description
 * things with converting skin, mainly:
 * csv(lr2skin) -> xml
 * xml -> lua
 * - @lazykuna
 *
 * converting LR2 file is written at skinlr2.cpp
 */

#pragma once

#include "skin.h"
#include "tinyxml2.h"
#include <map>
#include <vector>

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

	struct OP {
		int op[4];
	};

private:
	Skin *s;
	SkinMetric *sm;
	tinyxml2::XMLElement *cur_e;				// current base element
	std::vector<line_v_> lines_;				// contains line data
	std::vector<args_v_> line_args_;			// contains arguments per line
	int line_total;								// the line we totally read
	char filepath[1024];						// current file name

	/*
	* parsing status
	*/
	int image_cnt;												// only for setting image name
	int font_cnt;												// only for setting font name
	std::map<int, int> texturefont_id;							// check texturefont existence

	int currentline;
	class Resource {
		struct Font {
			const char* path;
			int size;
			int thick;
		};
		std::vector<Font> font;
		std::vector<std::string> tfont;
		std::vector<std::string> image;
		int fntattr_idx;
		int fntpath_idx;
	public:
		void AddImage(const char* path);
		void AddText(const char* path);
		void AddTextAttr(int size, int thick);
		void AddTFont(const char* data);
		std::string ToString();
		void Clear();
	};
	Resource m_Res;

	/*
	* local translator
	*/
	int GenerateTexturefontString(tinyxml2::XMLElement *srcelement);	// for #XXX_NUMBER object
	void ConvertToTextureFont(tinyxml2::XMLElement *numele);

	/*
	* Inner skin loaders
	*/
	// load and stores skin in splited line data
	int LoadSkin(const char *filepath);
	void SplitCSVLine(char *p, const char **args);
	// parse all loaded skin
	void ParseSkinCSVLine();
	void ParseSkinMetricLine();

	bool ProcessDepreciated(const args_read_& args);
	bool ProcessCondition(const args_read_& args);
	bool IsMetadata(const char* cmd);
	bool ProcessMetadata(const args_read_& args);
	bool ProcessResource(const args_read_& args);
	void ProcessNumber(tinyxml2::XMLElement* obj, int sop1, int sop2, int sop3);
	int ProcessLane(const args_read_& args, tinyxml2::XMLElement *src, int resid);			// process commands about lane
	int ProcessCombo(const args_read_& args, tinyxml2::XMLElement *obj);		// process commands about combo
	int ProcessSelectBar(const args_read_& args, tinyxml2::XMLElement *obj);	// process commands about select bar
	int ProcessSelectBar_DST(const args_read_& args);							// process commands about select bar
	// pacemaker: use default XML

	OP ProcessSRC(const args_read_& args);
	OP ProcessDST(const args_read_& args);
public:
	// this includes skin layout
	// use this method after LR2Skin is parsed.
	bool ParseCSV(const char* filepath, Skin *s);
	// this includes metadata
	// metadata should be saved as xml file.
	bool ParseLR2Skin(const char *filepath, SkinMetric *s);
	void SetSkinMetric(SkinMetric *s) { sm = s; };
	void Clear();

	_LR2SkinParser() { Clear(); }
};

namespace SkinConverter {
	bool ConvertLR2SkinToXml(const char* lr2skinpath);
	bool ConvertLR2SkinToLua(const char* lr2skinpath);
}

class XmlToLuaConverter {
private:
	// head: values which will be logically interpreted to create body
	std::string head;
	std::string body;
	// current indentation
	int indent;
	// current parse depth (now useless?)
	int depth;
	// current object id
	int objid;

	void AppendIndentBody(int indent);
	void AppendBody(const std::string& str);
	void AppendHead(const std::string& str);
	void AppendSRC(const tinyxml2::XMLElement *e);
	void AppendDST(const tinyxml2::XMLElement *dst);
	void AppendComment(const tinyxml2::XMLComment *cmt);
	void AppendObject(const tinyxml2::XMLElement *e, const char* name);
	void AppendElement(const tinyxml2::XMLElement *e);
public:
	void Parse(const tinyxml2::XMLNode *node);
	void StartParse(const tinyxml2::XMLNode *node);
	std::string GetLuaCode();	// call after parse
	void Clear();				// called automatically
};

// enable this line if you have lua library
#include "LuaManager.h"

namespace SkinTest {
#ifdef LUAMANAGER
	void InitLua();
	void CloseLua();
	bool TestLuaSkin(const char* luaskinpath);
#endif
}