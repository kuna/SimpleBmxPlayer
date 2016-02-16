#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
using namespace tinyxml2;
//
// skin converter
//


namespace SkinConverter {
	// private function; for searching including file
	void SearchInclude(std::vector<std::string>& inc, XMLElement* node) {
		for (XMLElement* n = node;
			n;
			n = n->NextSiblingElement())
		{
			if (strcmp(n->Name(), "include") == 0) {
				inc.push_back(SkinUtil::GetAbsolutePath(n->Attribute("path")));
			}
			SearchInclude(inc, n->FirstChildElement());
		}
	}

	// depreciated; only for basic test, or 
	bool ConvertLR2SkinToXml(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".xml");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->Save(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".xml");
					// TODO: depart texturefont files?
					csvskin->Save(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return r;
	}

	bool ConvertLR2SkinToLua(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".lua");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->SaveToLua(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".lua");
					// TODO: depart texturefont files?
					csvskin->SaveToLua(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return false;
	}
}