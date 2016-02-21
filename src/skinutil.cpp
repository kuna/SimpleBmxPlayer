#include "skinutil.h"

// temporary buffer used for dividing classes
char buffer[10240];

namespace SkinUtil {
	ConditionAttribute::ConditionAttribute() {}
	ConditionAttribute::ConditionAttribute(const char *classnames) {
		AddCondition(classnames);
	}

	void ConditionAttribute::AddCondition(const char *cond) {
		if (!IsConditionExists(cond))
			classes.insert(std::pair<std::string, int>(cond, 0));
	}

	void ConditionAttribute::AddConditions(const char *classnames) {
		strcpy(buffer, classnames);
		char *p = buffer;
		char *cur = p;
		while (p != 0) {
			p = strchr(p, ' ');
			if (p != 0) *p = 0;
			AddCondition(cur);
			if (!p) break;
			cur = ++p;
		}
	}

	void ConditionAttribute::RemoveCondition(const char *classnames) {
		strcpy(buffer, classnames);
		char *p = buffer;
		char *cur = p;
		while (p != 0) {
			p = strchr(p, ' ');
			if (p != 0) *p = 0;
			if (IsConditionExists(cur)) {
				classes.erase(classes.find(cur));
			}
			if (!p) break;
			cur = ++p;
		}
	}

	int ConditionAttribute::GetConditionNumber() {
		return classes.size();
	}

	void ConditionAttribute::CheckCondition(const char *classname) {
		if (IsConditionExists(classname))
			classes.find(classname)->second = 1;
	}

	void ConditionAttribute::UnCheckCondition(const char *classname) {
		if (IsConditionExists(classname))
			classes.find(classname)->second = 0;
	}

	bool ConditionAttribute::IsConditionExists(const char *classname) {
		return (classes.find(classname) != classes.end());
	}

	const char *ConditionAttribute::ToString() {
		strcpy(buffer, "");
		for (auto it = classes.begin(); it != classes.end(); ++it) {
			strcat(buffer, it->first.c_str());
			strcat(buffer, ",");
		}
		if (classes.size() > 0)
			buffer[strlen(buffer) - 1] = 0;
		return buffer;
	}

	XMLElement* FindElement(XMLElement *parent, const char* elementname, XMLDocument* createIfNotExists) {
		_ASSERT(parent);
		XMLElement *r = parent->FirstChildElement(elementname);
		if (!r && createIfNotExists) {
			r = createIfNotExists->NewElement(elementname);
			parent->LinkEndChild(r);
		}
		return r;
	}

	XMLElement* FindElementWithAttribute(XMLElement *parent, const char* elementname, const char *attribute, const char *value, XMLDocument* createIfNotExists) {
		_ASSERT(parent);
		XMLElement *r = parent->FirstChildElement(elementname);
		if (!r) {
			if (createIfNotExists) {
				r = createIfNotExists->NewElement(elementname);
				r->SetAttribute(attribute, value);
				parent->LinkEndChild(r);
				return r;
			}
			else {
				return 0;
			}
		}
		XMLElement *s = r;
		do {
			if (r->Attribute(attribute, value))
				return r;
			r = r->NextSiblingElement(elementname);
		} while (r != 0 && r != s);
		// cannot found previous element
		if (createIfNotExists) {
			r = createIfNotExists->NewElement(elementname);
			r->SetAttribute(attribute, value);
			parent->LinkEndChild(r);
			return r;
		}
		return 0;
	}

	XMLElement* FindElementWithAttribute(XMLElement *parent, const char* elementname, const char *attribute, int value, XMLDocument* createIfNotExists) {
		char v[12];
		itoa(value, v, 10);
		return FindElementWithAttribute(parent, elementname, attribute, v, createIfNotExists);
	}

	void ConvertLR2PathToRelativePath(std::string& lr2path) {
		int islr2path;
		islr2path = lr2path.find("LR2files");
		if (islr2path != std::string::npos) {
			ReplaceString(lr2path, "\\", "/");
			ReplaceString(lr2path, "./", "");
			ReplaceString(lr2path, "LR2files/Theme/", "");
			int p = lr2path.find('/');
			if (p != std::string::npos) {
				lr2path = "./" + lr2path.substr(p + 1);
				p = lr2path.find('/', 3);
				if (p != std::string::npos)
					lr2path = "./" + lr2path.substr(p + 1);
			}
		}
	}

	void GetParentDirectory(std::string& filepath) {
		ReplaceString(filepath, "\\", "/");
		int p = filepath.rfind('/');
		if (p) {
			filepath = filepath.substr(0, p);
		}
	}

	void ConvertRelativePathToAbsPath(std::string& relativepath, std::string& basepath) {
		ReplaceString(relativepath, "./", "");
		relativepath = basepath + "/" + relativepath;
	}

	void ReplaceString(std::string& source, std::string const& find, std::string const& replace) {
		for (std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
		{
			source.replace(i, find.length(), replace);
			i += replace.length();
		}
	}

	const char* FindString(const char *start, const char *target) {
		return strstr(start, target);
	}
}
