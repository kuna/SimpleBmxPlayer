#include "skin.h"
#include "skinutil.h"
using namespace SkinUtil;

// ---------------------------------------------------------

/*
 * Creates render tree for rendering
 * This also cuts and reduces class conditions
 */

bool Skin::Parse(const char *filepath) {
	if (!skinlayout.Parse(filepath)) {
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

	XMLElement *option = skinlayout.FirstChildElement("Option");
	XMLElement *ele_switch = option->FirstChildElement("CustomSwitch");
	while (ele_switch) {
		if (ele_switch->FirstChild()) {
			XMLElement *defaultoption = ele_switch->FirstChildElement();
			o->GetSwitches().push_back({ ele_switch->Attribute("name"), defaultoption->Attribute("value") });
		}
		ele_switch = ele_switch->NextSiblingElement("CustomSwitch");
	}

	/*
	 * CustomValue isn't supported in LR2 skin
	 */
	XMLElement *ele_value = option->FirstChildElement("CustomValue");
	while (ele_value) {
		if (ele_value->FirstChild()) {
			XMLElement *defaultoption = ele_value->FirstChildElement();
			o->GetValues().push_back({ ele_value->Attribute("name"), defaultoption->IntAttribute("value") });
		}
		ele_value = ele_value->NextSiblingElement("CustomValue");
	}

	XMLElement *ele_file = option->FirstChildElement("CustomFile");
	while (ele_file) {
		o->GetFiles().push_back({ ele_file->Attribute("name"), ele_file->Attribute("path") });
		ele_file = ele_file->NextSiblingElement("CustomFile");
	}
}

Skin::Skin(): skinlayout(false) {}
Skin::~Skin() { Release(); }

// --------------------- Skin End --------------------------
