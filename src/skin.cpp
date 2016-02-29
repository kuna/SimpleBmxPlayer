#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
using namespace SkinUtil;

// ---------------------------------------------------------

/*
 * Creates render tree for rendering
 * This also cuts and reduces class conditions
 */

bool Skin::Load(const char *filepath) {
	if (skinlayout.LoadFile(filepath) != 0) {
		return false;
	}
	return true;
}

bool Skin::Save(const char *filepath) {
	return (skinlayout.SaveFile(filepath) == XML_SUCCESS);
}






// ---------- STree Part begin ------------------------------
#if 0
#include "LuaHelper.h"
void SNode::ParseLua(lua_State *l) {
	if (!lua_istable(l, -1)) {
		printf("SNode Warning: Node must given as table, ignore.");
	}
	else {
		lua_pushnil(l);
		while (lua_next(l, -2)) {
			// get table key
			std::string key_;
			lua_pushvalue(l, -2);
			key_ = lua_tostring(l, -1);
			lua_pop(l, 1);

			// check element:
			// - child? event? attr?
			if (lua_istable(l, -1)) {
				// child - call recursive parser
				// (TODO)
				if (tree) {
					SNode *child = tree->NewNode();
					children_.push_back(child);
					child->ParseLua(l);
				}
				else {
					printf("child exists! but cannot parse because it's single node object\n");
				}
			} else {
				if (strncmp(key_.c_str(), "On", 2) == 0) {
					// if not compiled lua func, then attempt to compile it
					if (!lua_isfunction(l, -1)) {
						std::string luacode = lua_tostring(l, -1);
						lua_pop(l, 1);
						if (!LuaHelper::RunExpression(l, luacode)) {
							// if invalid func, then add nil value to avoid exception
							// COMMENT: nil is automatically added.
							//lua_pushnil(l);
						}
					}
					// register event handler
					if (lua_isfunction(l, -1)) {
						handler_[key_].SetFromStack(l);
						lua_pushnil(l);		// luaL_ref pops a stack
					}
				}
				// a normal attribute, but it isn't allowed if attribute name is number
				else if (lua_isnumber(l, -2)) {
					printf("SNode Warning: number attribute name is ignored.\n");
				}
				// generic attribute
				else {
					attribute_[key_] = lua_tostring(l, -1);
				}
			}
			lua_pop(l, 1);
		}
	}
}

void SNode::ParseXml(const XMLElement* e) {
	// TODO
}

// TODO: how to parse Src?
std::string SNode::NodeToString(bool recursive, int indent) {
	std::ostringstream ss;

	// indent
	for (int i = 0; i < indent; i++) {
		ss << "  ";
	}

	// parse all attribute
	ss << "Node attributes: ";
	for (auto it = attribute_.begin(); it != attribute_.end(); ++it) {
		ss << it->first << ",";
	}
	ss << "/events: " << handler_.size();
	ss << "/childs: " << children_.size();
	ss << "\n";
	if (recursive) {
		for (int i = 0; i < children_.size(); i++) {
			ss << children_[i]->NodeToString(recursive, indent + 1);
		}
	}

	return ss.str();
}

SNode* STree::NewNode() {
	SNode* tmp = new SNode(this);
	nodepool_.push_back(tmp);
	return tmp;
}

void STree::ClearTree() {
	for (int i = 0; i < nodepool_.size(); i++)
		delete nodepool_[i];
	nodepool_.clear();
}
#endif
// ---------- STree part end --------------------------------

// https://eliasdaler.wordpress.com/2014/11/01/using-lua-with-c-luabridge-part-2-using-scripts-for-object-behaviour/






// ---------- Skin part begin -------------------------------

bool Skin::SaveToLua(const char* filepath) {
	// convert first
	XmlToLuaConverter *converter = new XmlToLuaConverter();
	converter->StartParse(skinlayout.FirstChild());		// (doc)->Skin->(element)
	std::string out = converter->GetLuaCode();
	delete converter;

	// write start
	FILE *f = SkinUtil::OpenFile(filepath, "w");
	if (!f) return false;
	fwrite(out.c_str(), 1, out.size(), f);
	fclose(f);
	return true;
}

void Skin::Clear() {
	skinlayout.Clear();
}

Skin::Skin(): skinlayout(false) {}
Skin::~Skin() { Clear(); }

// --------------------- Skin End --------------------------

SkinMetric::SkinMetric(): tree(false) {}

SkinMetric::~SkinMetric() { }

void SkinMetric::GetDefaultOption(SkinOption *o) {
	o->Clear();

	XMLElement *skin = tree.FirstChildElement("skin");
	XMLElement *option = skin->FirstChildElement("option");
	XMLElement *ele_switch = option->FirstChildElement("customswitch");
	while (ele_switch) {
		if (ele_switch->FirstChild()) {
			XMLElement *defaultoption = ele_switch->FirstChildElement();
			o->GetSwitches().push_back({ ele_switch->Attribute("name"), defaultoption->Attribute("value") });
		}
		ele_switch = ele_switch->NextSiblingElement("customswitch");
	}

	/*
	* CustomValue isn't supported in LR2 skin (dummy code here)
	*/
	XMLElement *ele_value = option->FirstChildElement("customvalue");
	while (ele_value) {
		if (ele_value->FirstChild()) {
			XMLElement *defaultoption = ele_value->FirstChildElement();
			o->GetValues().push_back({ ele_value->Attribute("name"), defaultoption->IntAttribute("value") });
		}
		ele_value = ele_value->NextSiblingElement("customvalue");
	}

	XMLElement *ele_file = option->FirstChildElement("customfile");
	while (ele_file) {
		o->GetFiles().push_back({ ele_file->Attribute("name"), ele_file->Attribute("path") });
		ele_file = ele_file->NextSiblingElement("customfile");
	}
}

bool SkinMetric::Save(const char *filepath) {
	return (tree.SaveFile(filepath) == XML_SUCCESS);
}

bool SkinMetric::Load(const char* filename) {
	return (tree.LoadFile(filename) == XML_SUCCESS);
}

void SkinMetric::Clear() {
	tree.Clear();
}