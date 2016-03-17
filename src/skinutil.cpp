#include "skinutil.h"
#include <algorithm>

using namespace std;

// temporary buffer used for dividing classes
char buffer[10240];

namespace SkinUtil {
	ConditionAttribute::ConditionAttribute() {}
	ConditionAttribute::ConditionAttribute(const char *classnames) {
		AddCondition(classnames);
	}

	void ConditionAttribute::AddCondition(const char *cond) {
		if (!cond || *cond == 0 || strcmp(cond, "true") == 0) return;
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




	void TweenCommand::Add(const std::string& cmd, const std::string& val) {
		cmds.push_back(cmd + ":" + val);
	}
	void TweenCommand::Add(const std::string& cmd, int val) {
		char _n[12];
		itoa(val, _n, 10);
		std::string n = cmd + ":" + _n;
		cmds.push_back(n);
	}

	void TweenCommand::Parse(const std::string& cmd) {
		cmds.clear();
		int b = 0, i = 0;
		while ((i = cmd.find_first_of(",;", b)) != std::string::npos) {
			std::string n = cmd.substr(b, i - b);
			if (n.size()) {
				cmds.push_back(n);
			}
			b = i + 1;
		}
	}
	std::string TweenCommand::ToString() {
		std::string r;
		for (auto it = Begin(); it != End(); ++it) {
			r += *it + ",";
		}
		if (r.back() == ',') r.pop_back();
		return r;
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
			// remove first folder
			ReplaceString(lr2path, "\\", "/");
			ReplaceString(lr2path, "./", "");
			ReplaceString(lr2path, "LR2files/Theme/", "");
			// remove 2 level folder, if exists
			int p = lr2path.find_first_of("/\\");
			if (p != std::string::npos) {
				lr2path = "./" + lr2path.substr(p + 1);
				p = lr2path.find_first_of("/\\");
				if (p != std::string::npos)
					lr2path = "./" + lr2path.substr(p + 1);
			}
		}
	}

	std::string GetParentDirectory(const std::string& filepath) {
		int p = filepath.find_last_of("/\\");
		if (p) {
			return filepath.substr(0, p);
		}
		else return filepath;
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

	std::string ReplaceExtension(const std::string& filepath, const std::string& ext) {
		return filepath.substr(0, filepath.find_last_of(".")) + ext;
	}

	/* 
	 * working path related
	 */
	std::string basepath = "";		// should end with separator (slash)
	std::string GetAbsolutePath(const std::string& relpath) {
		// it's already absolute path
		if (relpath.size() && (relpath[0] == '/' || relpath[1] == ':'))
			return relpath;
		if (strncmp(relpath.c_str(), "./", 2) == 0) return basepath + relpath.substr(2);
		else return basepath + relpath;
	}

	void SetBasePath(const std::string& path) {
		basepath = path;
		if (basepath.back() != '/')
			basepath.push_back('/');
	}

	const char* FindString(const char *start, const char *target) {
		return strstr(start, target);
	}







	const wchar_t INVALID_CHAR = 0xFFFD; /* U+FFFD REPLACEMENT CHARACTER */

	/* Given a UTF-8 byte, return the length of the codepoint (if a start code)
	* or 0 if it's a continuation byte. */
	int utf8_get_char_len(char p)
	{
		if (!(p & 0x80)) return 1; /* 0xxxxxxx - 1 */
		if (!(p & 0x40)) return 1; /* 10xxxxxx - continuation */
		if (!(p & 0x20)) return 2; /* 110xxxxx */
		if (!(p & 0x10)) return 3; /* 1110xxxx */
		if (!(p & 0x08)) return 4; /* 11110xxx */
		if (!(p & 0x04)) return 5; /* 111110xx */
		if (!(p & 0x02)) return 6; /* 1111110x */
		return 1; /* 1111111x */
	}

	static inline bool is_utf8_continuation_byte(char c)
	{
		return (c & 0xC0) == 0x80;
	}

	/* Decode one codepoint at start; advance start and place the result in ch.
	* If the encoded string is invalid, false is returned. */
	bool utf8_to_wchar_ec(const string &s, unsigned &start, wchar_t &ch)
	{
		if (start >= s.size())
			return false;

		if (is_utf8_continuation_byte(s[start]) || /* misplaced continuation byte */
			(s[start] & 0xFE) == 0xFE) /* 0xFE, 0xFF */
		{
			start += 1;
			return false;
		}

		int len = utf8_get_char_len(s[start]);

		const int first_byte_mask[] = { -1, 0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

		ch = wchar_t(s[start] & first_byte_mask[len]);

		for (int i = 1; i < len; ++i)
		{
			if (start + i >= s.size())
			{
				/* We expected a continuation byte, but didn't get one. Return error, and point
				* start at the unexpected byte; it's probably a new sequence. */
				start += i;
				return false;
			}

			char byte = s[start + i];
			if (!is_utf8_continuation_byte(byte))
			{
				/* We expected a continuation byte, but didn't get one. Return error, and point
				* start at the unexpected byte; it's probably a new sequence. */
				start += i;
				return false;
			}
			ch = (ch << 6) | (byte & 0x3F);
		}

		bool bValid = true;
		{
			unsigned c1 = (unsigned)s[start] & 0xFF;
			unsigned c2 = (unsigned)s[start + 1] & 0xFF;
			int c = (c1 << 8) + c2;
			if ((c & 0xFE00) == 0xC000 ||
				(c & 0xFFE0) == 0xE080 ||
				(c & 0xFFF0) == 0xF080 ||
				(c & 0xFFF8) == 0xF880 ||
				(c & 0xFFFC) == 0xFC80)
			{
				bValid = false;
			}
		}

		if (ch == 0xFFFE || ch == 0xFFFF)
			bValid = false;

		start += len;
		return bValid;
	}

	/* Like utf8_to_wchar_ec, but only does enough error checking to prevent crashing. */
	bool utf8_to_wchar(const char *s, size_t iLength, unsigned &start, wchar_t &ch)
	{
		if (start >= iLength)
			return false;

		int len = utf8_get_char_len(s[start]);

		if (start + len > iLength)
		{
			// We don't have room for enough continuation bytes. Return error.
			start += len;
			ch = L'?';
			return false;
		}

		switch (len)
		{
		case 1:
			ch = (s[start + 0] & 0x7F);
			break;
		case 2:
			ch = ((s[start + 0] & 0x1F) << 6) |
				(s[start + 1] & 0x3F);
			break;
		case 3:
			ch = ((s[start + 0] & 0x0F) << 12) |
				((s[start + 1] & 0x3F) << 6) |
				(s[start + 2] & 0x3F);
			break;
		case 4:
			ch = ((s[start + 0] & 0x07) << 18) |
				((s[start + 1] & 0x3F) << 12) |
				((s[start + 2] & 0x3F) << 6) |
				(s[start + 3] & 0x3F);
			break;
		case 5:
			ch = ((s[start + 0] & 0x03) << 24) |
				((s[start + 1] & 0x3F) << 18) |
				((s[start + 2] & 0x3F) << 12) |
				((s[start + 3] & 0x3F) << 6) |
				(s[start + 4] & 0x3F);
			break;

		case 6:
			ch = ((s[start + 0] & 0x01) << 30) |
				((s[start + 1] & 0x3F) << 24) |
				((s[start + 2] & 0x3F) << 18) |
				((s[start + 3] & 0x3F) << 12) |
				((s[start + 4] & 0x3F) << 6) |
				(s[start + 5] & 0x3F);
			break;

		}

		start += len;
		return true;
	}


	// UTF-8 encode ch and append to out.
	void wchar_to_utf8(wchar_t ch, string &out)
	{
		if (ch < 0x80) { out.append(1, (char)ch); return; }

		int cbytes = 0;
		if (ch < 0x800) cbytes = 1;
		else if (ch < 0x10000)    cbytes = 2;
		else if (ch < 0x200000)   cbytes = 3;
		else if (ch < 0x4000000)  cbytes = 4;
		else cbytes = 5;

		{
			int shift = cbytes * 6;
			const int init_masks[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
			out.append(1, (char)(init_masks[cbytes - 1] | (ch >> shift)));
		}

		for (int i = 0; i < cbytes; ++i)
		{
			int shift = (cbytes - i - 1) * 6;
			out.append(1, (char)(0x80 | ((ch >> shift) & 0x3F)));
		}
	}

	wchar_t utf8_get_char(const string &s)
	{
		unsigned start = 0;
		wchar_t ret;
		if (!utf8_to_wchar_ec(s, start, ret))
			return INVALID_CHAR;
		return ret;
	}

	// Replace invalid sequences in s.
	void utf8_sanitize(string &s)
	{
		string ret;
		for (unsigned start = 0; start < s.size();)
		{
			wchar_t ch;
			if (!utf8_to_wchar_ec(s, start, ch))
				ch = INVALID_CHAR;

			wchar_to_utf8(ch, ret);
		}

		s = ret;
	}

	bool utf8_is_valid(const string &s)
	{
		for (unsigned start = 0; start < s.size();)
		{
			wchar_t ch;
			if (!utf8_to_wchar_ec(s, start, ch))
				return false;
		}
		return true;
	}


	wstring RStringToWstring(const string &s)
	{
		wstring ret;
		ret.reserve(s.size());
		for (unsigned start = 0; start < s.size();)
		{
			char c = s[start];
			if (!(c & 0x80))
			{
				// ASCII fast path
				ret += c;
				++start;
				continue;
			}

			wchar_t ch = L'\0';
			if (!utf8_to_wchar(s.data(), s.size(), start, ch))
				ch = INVALID_CHAR;
			ret += ch;
		}

		return ret;
	}

	string WStringToRString(const wstring &sStr)
	{
		string sRet;

		for (unsigned i = 0; i < sStr.size(); ++i)
			wchar_to_utf8(sStr[i], sRet);

		return sRet;
	}





	/*
	 * I/O
	 */

	FILE* OpenFile(const char* path, const char* mode) {
#ifdef _WIN32
		wstring wpath = RStringToWstring(path);
		wstring wmode = RStringToWstring(mode);
		FILE *f = 0;
		if (_wfopen_s(&f, wpath.c_str(), wmode.c_str()) == 0)
			return f;
		else
			return 0;
#else
		return fopen(path, mode);
#endif
	}
}
std::string ssprintf(const char *fmt, ...)
{
	va_list	va;
	va_start(va, fmt);
	return vssprintf(fmt, va);
}

#define FMT_BLOCK_SIZE		2048 // # of bytes to increment per try

std::string vssprintf(const char *szFormat, va_list argList)
{
	std::string sStr;

#if defined(WIN32)
	char *pBuf = NULL;
	int iChars = 1;
	int iUsed = 0;
	int iTry = 0;

	do
	{
		// Grow more than linearly (e.g. 512, 1536, 3072, etc)
		iChars += iTry * FMT_BLOCK_SIZE;
		pBuf = (char*)_alloca(sizeof(char)*iChars);
		iUsed = vsnprintf(pBuf, iChars - 1, szFormat, argList);
		++iTry;
	} while (iUsed < 0);

	// assign whatever we managed to format
	sStr.assign(pBuf, iUsed);
#else
	static bool bExactSizeSupported;
	static bool bInitialized = false;
	if (!bInitialized)
	{
		/* Some systems return the actual size required when snprintf
		* doesn't have enough space.  This lets us avoid wasting time
		* iterating, and wasting memory. */
		char ignore;
		bExactSizeSupported = (snprintf(&ignore, 0, "Hello World") == 11);
		bInitialized = true;
	}

	if (bExactSizeSupported)
	{
		va_list tmp;
		va_copy(tmp, argList);
		char ignore;
		int iNeeded = vsnprintf(&ignore, 0, szFormat, tmp);
		va_end(tmp);

		char *buf = new char[iNeeded + 1];
		std::fill(buf, buf + iNeeded + 1, '\0');
		vsnprintf(buf, iNeeded + 1, szFormat, argList);
		RString ret(buf);
		delete[] buf;
		return ret;
	}

	int iChars = FMT_BLOCK_SIZE;
	int iTry = 1;
	for (;;)
	{
		// Grow more than linearly (e.g. 512, 1536, 3072, etc)
		char *buf = new char[iChars];
		std::fill(buf, buf + iChars, '\0');
		int used = vsnprintf(buf, iChars - 1, szFormat, argList);
		if (used == -1)
		{
			iChars += (++iTry * FMT_BLOCK_SIZE);
		}
		else
		{
			/* OK */
			sStr.assign(buf, used);
		}

		delete[] buf;
		if (used != -1)
		{
			break;
		}
	}
#endif
	return sStr;
}