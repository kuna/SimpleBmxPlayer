#pragma once

#include "tinyxml2.h"

using namespace tinyxml2;

#include <map>

namespace SkinUtil {
	/*
	 * This class is for ease of editing class string
	 * This class is used in SkinParser/Rendering engine.
	 */
	class ClassAttribute {
	private:
		std::map<std::string, int> classes;
	public:
		ClassAttribute();
		ClassAttribute(const char *classnames);
		const char *ToString();
		bool IsClassExists(const char *classname);

		void AddClass(const char *classnames);
		void RemoveClass(const char *classnames);
		int GetClassNumber();

		void CheckClass(const char *classname);
		void UnCheckClass(const char *classname);
		bool IsAllClassSelected();
		bool IsAnyClassSelected();
	};

	// general util
	XMLElement* FindElement(XMLElement *parent, const char *elementname, XMLDocument* createIfNotExists = 0);
	XMLElement* FindElementWithAttribute(XMLElement *parent, const char *elementname, const char *attribute, const char *value, XMLDocument* createIfNotExists = 0);
	XMLElement* FindElementWithAttribute(XMLElement *parent, const char *elementname, const char *attribute, int value, XMLDocument* createIfNotExists = 0);

	// util about modifying string/path
	void ConvertLR2PathToRelativePath(std::string& lr2path);
	void GetParentDirectory(std::string& filepath);
	void ConvertRelativePathToAbsPath(std::string& relativepath, std::string& basepath);
	void ReplaceString(std::string& source, std::string const& find, std::string const& replace);
	const char* FindString(const char *start, const char *target);

	// util about I/O
	bool IsFileExists(const char* path);
}