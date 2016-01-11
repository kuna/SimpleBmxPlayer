#include "skinoption.h"
#include <tinyxml2.h>		// we store & save skin state as XML file.
using namespace tinyxml2;

bool SkinOption::LoadSkinOption(const char *filepath) {
	XMLDocument *doc = new XMLDocument();
	if (doc->LoadFile(filepath)) {
		delete doc;
		return false;
	}
	XMLElement *options = doc->FirstChildElement("Options");

	XMLElement *ele_switch = options->FirstChildElement("Switch");
	while (ele_switch) {
		switches.push_back({ ele_switch->Attribute("name"), ele_switch->Attribute("value") });
		ele_switch = ele_switch->NextSiblingElement("Switch");
	}

	XMLElement *ele_value = options->FirstChildElement("Value");
	while (ele_switch) {
		values.push_back({ ele_switch->Attribute("name"), ele_switch->IntAttribute("value") });
		ele_switch = ele_switch->NextSiblingElement("Value");
	}

	XMLElement *ele_fileh = options->FirstChildElement("File");
	while (ele_switch) {
		files.push_back({ ele_switch->Attribute("name"), ele_switch->Attribute("value") });
		ele_switch = ele_switch->NextSiblingElement("File");
	}

	delete doc;
	return true;
}

bool SkinOption::SaveSkinOption(const char *filepath) {
	// create new doc
	XMLDocument *doc = new XMLDocument();
	XMLElement *options = doc->NewElement("Options");
	doc->LinkEndChild(options);

	for (auto it = switches.begin(); it != switches.end(); ++it) {
		XMLElement *ele_switch = doc->NewElement("Switch");
		ele_switch->SetAttribute("name", it->optionname.c_str());
		ele_switch->SetAttribute("value", it->switchname.c_str());
		options->LinkEndChild(ele_switch);
	}
	for (auto it = values.begin(); it != values.end(); ++it) {
		XMLElement *ele_switch = doc->NewElement("Value");
		ele_switch->SetAttribute("name", it->optionname.c_str());
		ele_switch->SetAttribute("value", it->value);
		options->LinkEndChild(ele_switch);
	}
	for (auto it = files.begin(); it != files.end(); ++it) {
		XMLElement *ele_switch = doc->NewElement("File");
		ele_switch->SetAttribute("name", it->optionname.c_str());
		ele_switch->SetAttribute("path", it->path.c_str());
		options->LinkEndChild(ele_switch);
	}

	if (doc->SaveFile(filepath)) {
		delete doc;
		return false;
	}

	delete doc;
	return true;
}

void SkinOption::Clear() {
	switches.clear();
	values.clear();
	files.clear();
}

std::vector<SkinOption::CustomTimer>& SkinOption::GetSwitches() {
	return switches;
}

std::vector<SkinOption::CustomValue>& SkinOption::GetValues() {
	return values;
}

std::vector<SkinOption::CustomFile>& SkinOption::GetFiles() {
	return files;
}