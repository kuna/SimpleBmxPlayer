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
}