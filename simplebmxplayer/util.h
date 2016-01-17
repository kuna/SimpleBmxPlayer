#pragma once

#include <string>
#include <vector>
#include "global.h"
using namespace std;

#ifdef _WIN32
#define PATH_SEPARATORW			L"\\"
#define PATH_SEPARATOR_WCHAR	L'\\'
#define PATH_SEPARATOR			"\\"
#define PATH_SEPARATOR_CHAR		'\\'
#else
#define PATH_SEPARATORW			L"/"
#define PATH_SEPARATOR_WCHAR	L'/'
#define PATH_SEPARATOR			"/"
#define PATH_SEPARATOR_CHAR		'/'
#endif

#define NOT(v) (!(v))
std::wstring to_wstring(int i);

namespace IO {
	std::string get_fileext(const std::string& filepath);
	std::string get_filedir(const std::string& filepath);
	std::string get_filename(const std::string& filepath);
	std::string substitute_extension(const std::string& filepath, const std::string& newext);
	std::string substitute_filename(const std::string& filepath, const std::string& newname);	// excludes extension
	bool is_file_exists(const std::string& filename);

#ifdef USE_MBCS	
	std::wstring get_fileext(const std::wstring& filepath);
	std::wstring get_filedir(const std::wstring& filepath);
	std::wstring substitute_extension(const std::wstring& filepath, const std::wstring& newext);
	std::wstring substitute_filename(const std::wstring& filepath, const std::wstring& newname);	// excludes extension
	std::wstring get_filename(const std::wstring& filepath);
	bool is_file_exists(const std::wstring& filename);
	bool is_directory_exists(const std::wstring& dirpath);
	std::wstring get_parentdir(const std::wstring& dirpath);
	bool create_directory(const std::wstring& filepath);
	bool make_parent_directory_recursive(const std::wstring& filepath);
	std::wstring make_filename_safe(const std::wstring& filepath);
#endif
}

namespace ENCODING {
	bool wchar_to_utf8(const wchar_t *org, char *out, int maxlen);
	bool utf8_to_wchar(const char *org, wchar_t *out, int maxlen);
}

// et-cetera
namespace COMMON {
	void lower(std::wstring& str);
	void lower(std::string& str);
	void upper(std::wstring& str);
	void upper(std::string& str);
}

// RString related
RString ssprintf(const char *fmt, ...);
RString vssprintf(const char *fmt, va_list argList);

RString join(const RString &sDelimitor, const vector<RString>& sSource);
RString join(const RString &sDelimitor, vector<RString>::const_iterator begin, vector<RString>::const_iterator end);

// Splits a RString into an vector<RString> according the Delimitor.
void split(const RString &sSource, const RString &sDelimitor, vector<RString>& asAddIt, const bool bIgnoreEmpty = true);
void split(const wstring &sSource, const wstring &sDelimitor, vector<wstring> &asAddIt, const bool bIgnoreEmpty = true);

/* In-place split. */
void split(const RString &sSource, const RString &sDelimitor, int &iBegin, int &iSize, const bool bIgnoreEmpty = true);
void split(const wstring &sSource, const wstring &sDelimitor, int &iBegin, int &iSize, const bool bIgnoreEmpty = true);

/* In-place split of partial string. */
void split(const RString &sSource, const RString &sDelimitor, int &iBegin, int &iSize, int iLen, const bool bIgnoreEmpty); /* no default to avoid ambiguity */
void split(const wstring &sSource, const wstring &sDelimitor, int &iBegin, int &iSize, int iLen, const bool bIgnoreEmpty);

void TrimLeft(RString &sStr, const char *szTrim = "\r\n\t ");
void TrimRight(RString &sStr, const char *szTrim = "\r\n\t ");
void Trim(RString &sStr, const char *szTrim = "\r\n\t ");
void StripCrnl(RString &sStr);
bool BeginsWith(const RString &sTestThis, const RString &sBeginning);
bool EndsWith(const RString &sTestThis, const RString &sEnding);
RString URLEncode(const RString &sStr);

/* encoding related */
int utf8_get_char_len(char p);
bool utf8_to_wchar(const char *s, size_t iLength, unsigned &start, wchar_t &ch);
bool utf8_to_wchar_ec(const RString &s, unsigned &start, wchar_t &ch);
void wchar_to_utf8(wchar_t ch, RString &out);
wchar_t utf8_get_char(const RString &s);
bool utf8_is_valid(const RString &s);
void utf8_remove_bom(RString &s);

/* IO Related */
bool GetFileContents(const RString &sPath, RString &sOut, bool bOneLine = false);
bool GetFileContents(const RString &sFile, vector<RString> &asOut);
