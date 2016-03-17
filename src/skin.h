/*
 * for converting LR2 skin
 */

#pragma once

#include <vector>
#include <map>
#include "tinyxml2.h"

/*
 * @description
 * stores XML-based skin information
 * (#IMAGE, #SRC_XXX, #DST_XXX ...)
 */
class Skin {
public:
	// very basic metadata
	char filepath[1024];

	// skin layout data is here
	tinyxml2::XMLDocument skinlayout;
public:
	Skin();
	~Skin();
	void Clear();
	// load skin xml file
	bool Load(const char* filepath);
	// load & parse lua
	bool LoadLua(const char* filepath);
	// save skin file
	bool Save(const char* filepath);
	// save skin file as lua code
	bool SaveToLua(const char* filepath);
	tinyxml2::XMLElement* GetBaseElement() { return skinlayout.FirstChildElement(); }
};

/*
 * @description
 * stores skin metric information
 * (#INFORMATION, #CUSTOMFILE, #CUSTOMOPTION, ...)
 */
class SkinMetric {
public:
	/*
	 * Switch value
	 * (split with semicolons)
	 */
	typedef struct {
		int value;					// value of <optionname> metrics.
		std::string desc;			// general desc for displaying
		std::string eventname;		// triggers event if it's necessary.
	} Option;
	typedef struct {
		std::string optionname;
		std::string desc;
		std::vector<Option> options;
	} CustomValue;

	/*
	 * String value
	 */
	typedef struct {
		std::string optionname;
		std::string path;			// regex filter
		std::string path_default;	// default path
	} CustomFile;

	/*
	 * basic skin information
	 */
	typedef struct {
		std::string title;
		std::string artist;
		int width;
		int height;
	} Information;

	std::vector<CustomValue> values;
	std::vector<CustomFile> files;
	Information info;
public:
	SkinMetric();
	~SkinMetric();
	bool Load(const char* filename);
	bool Save(const char* filename);
	void Clear();

	// for converter convenience
	void conv_AddOption(const std::string name, int startnum, int count);
	void conv_AddFileOption(const std::string name, const std::string value, const std::string def);
};


/*
 * @description
 * parsed node by Lua/Xml
 * basic skin structure, but not an rendering tree.
 * independent to tinyxml2.
 * - it's future implementation, not using now.
 */
#if 0
#include "LuaManager.h"
#include "LuaHelper.h"

class STree;
class SNode {
	/*
	 * SNode only has attr, handler, child attribute.
	 * no other functions for rendering. inherit if you want to do so..
	 */
protected:
	std::map<std::string, std::string> attribute_;
	std::map<std::string, LuaFunc> handler_;
	std::vector<SNode*> children_;
	STree* tree;
public:
	SNode() : SNode(0) {}
	SNode(STree* t) : tree(t) {}
	~SNode() {}

	// make node by lua table (child is not parsed) (table MUST be on stack)
	void ParseLua(lua_State *l);
	// make node by xml table (child is not parsed)
	void ParseXml(const tinyxml2::XMLElement *e);
	// for info/debug
	int GetChildCount();
	std::string NodeToString(bool recursive = true, int indent = 0);
	// access to node
	template<class T>
	bool NodeQuery(const std::string& key, T& out);
	std::vector<SNode*>::const_iterator NodeBegin();
	std::vector<SNode*>::const_iterator NodeEnd();
};

class STree: public SNode {
	/*
	 * Manages SNode like objects
	 */
protected:
	std::vector<SNode*> nodepool_;
public:
	STree() : SNode(this) {}
	~STree() { ClearTree(); }

	virtual SNode* NewNode();
	virtual void ClearTree();
};
#endif