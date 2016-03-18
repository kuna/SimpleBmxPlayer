#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
using namespace SkinUtil;

#define SAFE_STRING(s) (s)?(s):"";

// ---------------------------------------------------------

/*
 * Creates render tree for rendering
 * This also cuts and reduces class conditions
 */

bool Skin::Load(const char *filepath) {
	Clear();
	if (skinlayout.LoadFile(filepath) != 0) {
		return false;
	}
	return true;
}

bool Skin::Save(const char *filepath) {
	return (skinlayout.SaveFile(filepath) == XML_SUCCESS);
}

/* Process lua table into an xml object. requires lua object. */
bool Skin::LoadLua(const char* filepath) {
	Clear();
	// TODO
	return false;
}

bool Skin::SaveAsLua(const char* filepath) {
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

bool Skin::SaveAsCsv(const char* filepath) {
	// TODO.

	// parse lua code to generate resource list

	// frame element is ignored

	// font attribute - cannot be restored ...?

	return false;
}

void Skin::Clear() {
	parent = 0;
	current = 0;
	parent_stack.clear();
	skinlayout.Clear();

	// create base skin template
	XMLComment *cmt = skinlayout.NewComment("Auto-generated code.\n"
		"Converted from LR2Skin.\n"
		"LR2 project origins from lavalse, All rights reserved.");
	skinlayout.LinkEndChild(cmt);

	current = parent = skinlayout.NewElement("frame");
	skinlayout.LinkEndChild(current);
}

Skin::Skin() : skinlayout(false) { Clear();  }
Skin::~Skin() { }


tinyxml2::XMLNode* Skin::CreateComment(const char* body) {
	XMLNode *n = skinlayout.NewComment(body);
	parent->LinkEndChild(n);
	return n;
}
tinyxml2::XMLElement* Skin::CreateElement(const char* name) {
	XMLElement* e = skinlayout.NewElement(name);
	parent->LinkEndChild(e);
	return current = e;
}
tinyxml2::XMLElement* Skin::GetCurrentElement() {
	return current;
}
tinyxml2::XMLElement* Skin::GetCurrentParent() {
	return current;
}
void Skin::PushParent(bool setcurasparent) {
	parent_stack.push_back(parent);
	if (setcurasparent)
		parent = current;
}
void Skin::PopParent() {
	if (parent_stack.size()) {
		printf("LR2Skin - Invalid #ENDIF found!\n");
		return;
	}
	parent = parent_stack.back();
	parent_stack.pop_back();
}
void Skin::SetCurrentElement(tinyxml2::XMLElement* e) {
	current = e;
}
void Skin::SetCurrentParent(tinyxml2::XMLElement* e) {
	parent = e;
}
tinyxml2::XMLElement* Skin::FindElement(const char* name, bool createifnull) {
	XMLElement *e = 0;
	if ((e = parent->FirstChildElement(name)) == 0 && createifnull) {
		e = skinlayout.NewElement(name);
		parent->LinkEndChild(e);
	}
	return e;
}
tinyxml2::XMLElement* Skin::FindElementWithAttr(const char* name, const char* attr, const char* val, bool createifnull) {
	XMLElement *e = 0;
	for (e = parent->FirstChildElement(name); e;
		e = e->NextSiblingElement(name))
	{
		if (e->Attribute(attr, val)) {
			// we found one!
			break;
		}
	}
	// not exists? then create one
	if (!e && createifnull) {
		e = skinlayout.NewElement(name);
		e->SetAttribute(attr, val);
		parent->LinkEndChild(e);
	}
	return e;
}
tinyxml2::XMLElement* Skin::FindElementWithAttr(const char* name, const char* attr, int val, bool createifnull) {
	char _t[12];
	itoa(val ,_t, 10);
	return FindElementWithAttr(name, attr, _t, createifnull);
}
void Skin::SetName(const std::string& name) {
	current->SetName(name.c_str());
}
void Skin::SetText(const std::string& text) {
	current->SetText(text.c_str());
}

template<>
void Skin::SetAttribute(const char* attrname, int val) {
	current->SetAttribute(attrname, val);
}
template<>
void Skin::SetAttribute(const char* attrname, double val) {
	current->SetAttribute(attrname, val);
}
template<>
void Skin::SetAttribute(const char* attrname, const std::string& val) {
	current->SetAttribute(attrname, val.c_str());
}
// well... this is a bad case actually?
template<>
void Skin::SetAttribute(const char* attrname, std::string val) {
	current->SetAttribute(attrname, val.c_str());
}
template<>
void Skin::SetAttribute(const char* attrname, const char* val) {
	current->SetAttribute(attrname, val);
}

template<>
int Skin::GetAttribute(const char* attrname) {
	return current->IntAttribute(attrname);
}
template<>
double Skin::GetAttribute(const char* attrname) {
	return current->FloatAttribute(attrname);
}
template<>
std::string Skin::GetAttribute(const char* attrname) {
	return SAFE_STRING(current->Attribute(attrname));
}
template<>
const char* Skin::GetAttribute(const char* attrname) {
	return SAFE_STRING(current->Attribute(attrname));
}

void Skin::DeleteAttribute(const char* attrname) {
	current->DeleteAttribute(attrname);
}
bool Skin::IsAttribute(const char* attrname) {
	return current->Attribute(attrname) != 0;
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

// --------------------- Skin End --------------------------

SkinMetric::SkinMetric() {}

SkinMetric::~SkinMetric() { }

bool SkinMetric::Save(const char *filepath) {
	XMLDocument doc(false);

	XMLElement *metrics = doc.NewElement("Metrics");
	doc.LinkEndChild(metrics);

	// skin information
	XMLElement *e_info = doc.NewElement("Info");
	metrics->LinkEndChild(e_info);
	e_info->SetAttribute("width", info.width);
	e_info->SetAttribute("height", info.height);
	e_info->SetAttribute("title", info.title.c_str());
	e_info->SetAttribute("artist", info.artist.c_str());

	//
	for (auto it = values.begin(); it != values.end(); ++it) {
		XMLElement *ele_switch = doc.NewElement("Option");
		ele_switch->SetAttribute("name", it->optionname.c_str());
		ele_switch->SetAttribute("desc", it->desc.c_str());
		metrics->LinkEndChild(ele_switch);
		for (auto it2 = it->options.begin(); it2 != it->options.end(); ++it2) {
			XMLElement *select = doc.NewElement("Select");
			ele_switch->LinkEndChild(select);
			select->SetAttribute("value", it2->value);
			select->SetAttribute("desc", it2->desc.c_str());
			select->SetAttribute("event", it2->eventname.c_str());
		}
	}
	for (auto it = files.begin(); it != files.end(); ++it) {
		XMLElement *e_option = doc.NewElement("OptionFile");
		e_option->SetAttribute("name", it->optionname.c_str());
		e_option->SetAttribute("path", it->path.c_str());
		e_option->SetAttribute("default", it->path_default.c_str());
		metrics->LinkEndChild(e_option);
	}

	return (doc.SaveFile(filepath) == XML_SUCCESS);
}

bool SkinMetric::Load(const char* filename) {
	XMLDocument doc(false);
	XMLElement *metrics;
	if (doc.LoadFile(filename) != XML_SUCCESS || (metrics = doc.FirstChildElement("Metrics")) == 0) return false;

	// clear first
	Clear();

	// skin information
	XMLElement *e_info = metrics->FirstChildElement("Info");
	info.width = e_info->IntAttribute("width");
	info.height = e_info->IntAttribute("height");
	info.title = SAFE_STRING(e_info->Attribute("title"));
	info.artist = SAFE_STRING(e_info->Attribute("artist"));

	// customvalue
	for (XMLElement *e_option = metrics->FirstChildElement("Option");
		e_option; e_option = e_option->NextSiblingElement("Option"))
	{
		CustomValue val;
		val.optionname = SAFE_STRING(e_option->Attribute("name"));
		val.desc = SAFE_STRING(e_option->Attribute("desc"));
		for (XMLElement *option_e = e_option->FirstChildElement("Select");
			option_e;
			option_e = option_e->NextSiblingElement("Select"))
		{
			Option option;
			option.value = option_e->IntAttribute("value");
			option.desc = SAFE_STRING(option_e->Attribute("desc"));
			option.eventname = SAFE_STRING(option_e->Attribute("event"));
			val.options.push_back(option);
		}
	}

	// customfile
	for (XMLElement *e_option = metrics->FirstChildElement("OptionFile");
		e_option; e_option = e_option->NextSiblingElement("OptionFile"))
	{
		CustomFile val;
		val.optionname = e_option->Attribute("name");
		val.path = e_option->Attribute("path");
		val.path_default = e_option->Attribute("default");
		files.push_back(val);
	}

	return true;
}

void SkinMetric::Clear() {
	info.title = "UNKNOWN";
	info.artist = "UNKNOWN";
	info.width = 1280;
	info.height = 760;
}