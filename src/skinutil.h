#pragma once

#include "tinyxml2.h"

using namespace tinyxml2;

#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <stdarg.h>

namespace SkinUtil {
	/*
	 * This class is for ease of editing class string
	 * This class is used in SkinParser/Rendering engine.
	 */
	class ConditionAttribute {
	private:
		std::map<std::string, int> classes;
	public:
		ConditionAttribute();
		ConditionAttribute(const char *cond);
		const char *ToString();
		bool IsConditionExists(const char *cond);

		void AddCondition(const char *cond);
		void AddConditions(const char *conds);
		void RemoveCondition(const char *conds);
		int GetConditionNumber();

		/* under these classes are depreciated - we're going to use Lua language */
		void CheckCondition(const char *conds);
		void UnCheckCondition(const char *conds);
		bool IsAllConditionSelected();
		bool IsAnyConditionSelected();
	};

	class TweenCommand {
		std::vector<std::string> cmds;
		std::string argv[10];
	public:
		void Add(const std::string& cmd, const std::string& val);
		void Add(const std::string& cmd, int val);

		typedef std::vector<std::string>::iterator Iterator;
		Iterator Begin() { cmds.begin(); }
		Iterator End() { cmds.end(); }

		void Parse(const std::string& cmd);
		std::string ToString();
	};

	// general util
	XMLElement* FindElement(XMLElement *parent, const char *elementname, XMLDocument* createIfNotExists = 0);
	XMLElement* FindElementWithAttribute(XMLElement *parent, const char *elementname, const char *attribute, const char *value, XMLDocument* createIfNotExists = 0);
	XMLElement* FindElementWithAttribute(XMLElement *parent, const char *elementname, const char *attribute, int value, XMLDocument* createIfNotExists = 0);

	// util about modifying string/path
	void ConvertLR2PathToRelativePath(std::string& lr2path);
	std::string GetParentDirectory(const std::string& filepath);
	void ConvertRelativePathToAbsPath(std::string& relativepath, std::string& basepath);
	void ReplaceString(std::string& source, std::string const& find, std::string const& replace);
	const char* FindString(const char *start, const char *target);
	std::string ReplaceExtension(const std::string& filepath, const std::string& ext);
	std::string GetAbsolutePath(const std::string& relpath);
	void SetBasePath(const std::string& basepath);

	// util about I/O
	bool IsFileExists(const char* path);
	FILE* OpenFile(const char* path, const char* mode);
}

std::string ssprintf(const char *fmt, ...);
std::string vssprintf(const char *szFormat, va_list argList);