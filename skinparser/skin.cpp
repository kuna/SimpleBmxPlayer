#include "skin.h"
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
	if (skinlayout.SaveFile(filepath))
		return true;
	return false;
}

void Skin::Release() {
	skinlayout.Clear();
}

void Skin::GetDefaultOption(SkinOption *o) {
	o->Clear();

	XMLElement *skin = skinlayout.FirstChildElement("skin");
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

Skin::Skin(): skinlayout(false) {}
Skin::~Skin() { Release(); }

// --------------------- Skin End --------------------------
