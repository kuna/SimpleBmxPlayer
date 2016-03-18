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
private:
	// very basic metadata
	char filepath[1024];

	// skin layout data is here
	tinyxml2::XMLDocument skinlayout;

	// for modifying
	tinyxml2::XMLElement* parent;		// currently focusing element
	tinyxml2::XMLElement* current;		// currently editing element
	std::vector<tinyxml2::XMLElement*> parent_stack;
public:
	Skin();
	~Skin();
	void Clear();

	// load & save xml
	bool Load(const char* filepath);
	bool Save(const char* filepath);
	// load & save lua
	bool LoadLua(const char* filepath);
	bool SaveAsLua(const char* filepath);
	// save as csv - Is it possible ...?
	bool SaveAsCsv(const char* filepath);

	// util for editing 
	tinyxml2::XMLElement* GetBaseElement() { return skinlayout.FirstChildElement(); }
	tinyxml2::XMLNode* CreateComment(const char* body);
	void SetName(const std::string& name);
	void SetText(const std::string& text);
	void DeleteNode(tinyxml2::XMLElement *e) { skinlayout.DeleteNode(e); }
	tinyxml2::XMLElement* CreateElement(const char* name);
	tinyxml2::XMLElement* GetCurrentElement();
	void SetCurrentElement(tinyxml2::XMLElement* e);
	tinyxml2::XMLElement* GetCurrentParent();
	void SetCurrentParent(tinyxml2::XMLElement* e);
	void PushParent(bool setcurasparent = true);
	void PopParent();
	tinyxml2::XMLElement* FindElement(const char* name, bool createifnull = false);
	tinyxml2::XMLElement* FindElementWithAttr(const char* name, const char* attr, const char* val, bool createifnull = false);
	tinyxml2::XMLElement* FindElementWithAttr(const char* name, const char* attr, int val, bool createifnull = false);
	template <typename T>
	void SetAttribute(const char* attrname, T val);
	template <typename T>
	T GetAttribute(const char* attrname);
	void DeleteAttribute(const char* attrname);
	bool IsAttribute(const char* attrname);
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
		std::string type;
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