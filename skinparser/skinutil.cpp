#include "skinutil.h"

// temporary buffer used for dividing classes
char buffer[10240];

namespace SkinUtil {
	ClassAttribute::ClassAttribute() {}
	ClassAttribute::ClassAttribute(const char *classnames) {
		AddClass(classnames);
	}

	void ClassAttribute::AddClass(const char *classnames) {
		strcpy(buffer, classnames);
		char *p = buffer;
		char *cur = p;
		while (p != 0) {
			p = strchr(p, ' ');
			if (p != 0) *p = 0;
			if (!IsClassExists(cur))
				classes.insert(std::pair<std::string, int>(cur, 0));
			if (!p) break;
			cur = ++p;
		}
	}

	void ClassAttribute::RemoveClass(const char *classnames) {
		strcpy(buffer, classnames);
		char *p = buffer;
		char *cur = p;
		while (p != 0) {
			p = strchr(p, ' ');
			if (p != 0) *p = 0;
			if (IsClassExists(cur)) {
				classes.erase(classes.find(cur));
			}
			if (!p) break;
			cur = ++p;
		}
	}

	int ClassAttribute::GetClassNumber() {
		return classes.size();
	}

	void ClassAttribute::CheckClass(const char *classname) {
		if (IsClassExists(classname))
			classes.find(classname)->second = 1;
	}

	void ClassAttribute::UnCheckClass(const char *classname) {
		if (IsClassExists(classname))
			classes.find(classname)->second = 0;
	}

	bool ClassAttribute::IsClassExists(const char *classname) {
		return (classes.find(classname) != classes.end());
	}

	const char *ClassAttribute::ToString() {
		strcpy(buffer, "");
		for (auto it = classes.begin(); it != classes.end(); ++it) {
			strcat(buffer, it->first.c_str());
			strcat(buffer, " ");
		}
		if (classes.size() > 0)
			buffer[strlen(buffer) - 1] = 0;
		return buffer;
	}
}
