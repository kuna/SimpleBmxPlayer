/*
 * for converting LR2 skin
 */

#pragma once

#include "skinoption.h"
#include "tinyxml2.h"
#include <vector>
#include <map>

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

	// render tree used for rendering
	//SkinRenderTree skinbody;
public:
	Skin();
	~Skin();
	void Release();
	// load skin xml file
	bool Load(const char* filepath);
	// save skin file
	bool Save(const char* filepath);
	// save skin file as lua code
	bool SaveToLua(const char* filepath);
};

/*
 * @description
 * only stores skin metric information
 * (#INFORMATION, #CUSTOMFILE, #CUSTOMOPTION, ...)
 */
class SkinMetric {
public:
	/*
	 * This class only provides method to converting lr2skin, not used in real game.
	 * So, we don't need to declare this structure productively; just store everything in xml object.
	 */
#if 0
	/*
	 * Switch value
	 * (split with semicolons)
	 */
	typedef struct {
		std::string optionnames;
		std::string switchnames;
	} CustomSwitch;

	/*
	 * Int value
	 */
	typedef struct {
		std::string optionname;
		int value;
	} CustomInt;
	void CreateCustomIntObject(XMLElement* base);

	/*
	 * String value
	 */
	typedef struct {
		std::string optionname;
		std::string path;
	} CustomFile;

	/*
	 * basic skin information
	 */
	typedef struct {

	} Information;

	std::vector<CustomSwitch> switches;
	std::vector<CustomInt> ints;
	std::vector<CustomFile> files;
	Information info;
#endif
	tinyxml2::XMLDocument tree;
public:
	SkinMetric();
	~SkinMetric();
	// save skin metric in xml formatted file
	bool Save(const char* filename);
	// get default useable option from this object ... 
	// COMMENT: this function shouldn't be in skinconverter. remove this.
	void GetDefaultOption(SkinOption *o);
};

/*
 * @description
 * parsed node by Lua/Xml
 * basic skin structure, but not an rendering tree.
 * independent to tinyxml2.
 */
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