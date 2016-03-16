#include "util.h"

#include "iconv\iconv.h"
#include <sstream>
#include <sys/stat.h>
#include <wchar.h>
#include <array>
#include <algorithm>
#include "md5.h"
#include "logger.h"
#include "file.h"
#include "zlib.h"		// to use compress / decompress function

using namespace std;

const wchar_t INVALID_CHAR = 0xFFFD; /* U+FFFD REPLACEMENT CHARACTER */

unsigned char g_UpperCase[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xF7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xFF,
};

unsigned char g_LowerCase[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xF7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xFF,
};


std::string get_fileext(const std::string& filepath) {
	auto i = filepath.find_last_of('.');
	if (i == std::wstring::npos)
		return "";
	return filepath.substr(i);
}

std::string get_filedir(const std::string& filepath) {
	return filepath.substr(0, filepath.find_last_of("/\\"));
}

std::string substitute_extension(const std::string& filepath, const std::string& newext) {
	auto i = filepath.find_last_of('.');
	if (i == std::wstring::npos)
		return filepath + newext;
	return filepath.substr(0, i) + newext;
}

std::string substitute_filename(const std::string& filepath, const std::string& newname) {
	auto i_start = filepath.find_last_of("/\\");
	if (i_start == std::wstring::npos)
		i_start = 0;
	auto i_end = filepath.find_last_of('.');
	if (i_end == std::wstring::npos)
		i_end = filepath.size() - 1;
	return filepath.substr(0, i_start) + PATH_SEPARATOR + newname + filepath.substr(i_end);
}

std::string get_filename(const std::string& filepath) {
	auto i = filepath.find_last_of("/\\");
	if (i == std::wstring::npos)
		return "";
	return filepath.substr(i + 1);
}

FILE* openfile(const std::string& filepath, const std::string& mode) {
#ifdef _WIN32
	FILE *fp;
	std::wstring filepath_w;
	std::wstring mode_w;
	filepath_w = RStringToWstring(filepath);
	mode_w = RStringToWstring(mode);
	if (_wfopen_s(&fp, filepath_w.c_str(), mode_w.c_str()) == 0)
		return fp;
	else
		return 0;
#else
	return fopen(filepath.c_str(), mode.c_str());
#endif
}

#ifdef USE_MBCS
std::wstring substitute_extension(const std::wstring& filepath, const std::wstring& newext) {
	auto i = filepath.find_last_of(L'.');
	if (i == std::wstring::npos)
		return filepath + newext;
	return filepath.substr(0, i) + newext;
}

std::wstring substitute_filename(const std::wstring& filepath, const std::wstring& newname) {
	auto i_start = filepath.find_last_of(L"/\\");
	if (i_start == std::wstring::npos)
		i_start = 0;
	auto i_end = filepath.find_last_of(L'.');
	if (i_end == std::wstring::npos)
		i_end = filepath.size() - 1;
	return filepath.substr(0, i_start) + PATH_SEPARATORW + newname + filepath.substr(i_end);
}

std::wstring get_fileext(const std::wstring& filepath) {
	auto i = filepath.find_last_of(L'.');
	if (i == std::wstring::npos)
		return L"";
	return filepath.substr(i);
}

std::wstring get_filedir(const std::wstring& filepath) {
	return filepath.substr(0, filepath.find_last_of(L"/\\"));
}

std::wstring get_filename(const std::wstring& filepath) {
	auto i = filepath.find_last_of(L"/\\");
	if (i == std::wstring::npos)
		return L"";
	return filepath.substr(i + 1);
}

bool is_file_exists(const std::wstring& filepath) {
	struct _stat64i32 s;
	if (_wstat(filepath.c_str(), &s) == 0) {
		if (s.st_mode & S_IFREG)
			return true;
		else
			return false;
	}
	else {
		return false;
	}
}

bool is_directory_exists(const std::wstring& dirpath) {
	struct _stat64i32 s;
	if (_wstat(dirpath.c_str(), &s) == 0) {
		if (s.st_mode & S_IFDIR)
			return true;
		else
			return false;
	}
	else {
		return false;
	}
}

std::wstring get_parentdir(const std::wstring& dirpath) {
	auto i = dirpath.find_last_of(L"/\\");
	if (i == std::wstring::npos)
		return L"";
	return dirpath.substr(0, i);
}

bool create_directory(const std::wstring& filepath) {
	return (_wmkdir(filepath.c_str()) == 0);
}

bool make_parent_directory_recursive(const std::wstring& filepath)
{
	// if current directory is not exist
	// then get parent directory
	// and check it recursively
	// after that, create own.
	if (is_directory_exists(filepath))
		return true;
	if (is_file_exists(filepath))
		return false;
	if (!create_directory(filepath.c_str())) {
		std::wstring parent = get_parentdir(filepath);
		if (NOT(make_parent_directory_recursive(parent))) {
			return false;
		}
		return create_directory(filepath.c_str());
	}
	else {
		return true;
	}
}

std::wstring make_filename_safe(const std::wstring& filepath) {
	std::wstring fn = get_filename(filepath);
#define REPLACESTR(s, o, r) (std::replace((s).begin(), (s).end(), (o), (r)))
	REPLACESTR(fn, L'/', L'_');
	if (_WIN32) {
		REPLACESTR(fn, L'|', L'_');
		REPLACESTR(fn, L'\\', L'_');
		REPLACESTR(fn, L':', L'_');
		REPLACESTR(fn, L'*', L'_');
		REPLACESTR(fn, L'<', L'_');
		REPLACESTR(fn, L'>', L'_');
	}
	return get_filedir(filepath) + PATH_SEPARATORW + fn;
}
#endif

/*
namespace ENCODING {
	bool wchar_to_utf8(const wchar_t *org, char *out, int maxlen)
	{
		iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
		if ((int)cd == -1)
			return false;

		out[0] = 0;
		const char *buf_iconv = (const char*)org;
		char *but_out_iconv = (char*)out;
		size_t len_in = wcslen(org) * 2;
		size_t len_out = maxlen;

		int r = iconv(cd, &buf_iconv, &len_in, &but_out_iconv, &len_out);
		if ((int)r == -1)
			return false;
		*but_out_iconv = 0;

		return true;
	}

	bool utf8_to_wchar(const char *org, wchar_t *out, int maxlen)
	{
		iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
		if ((int)cd == -1)
			return false;

		out[0] = 0;
		const char *buf_iconv = (const char*)org;
		char *but_out_iconv = (char*)out;
		size_t len_in = strlen(org);
		size_t len_out = maxlen * 2;

		int r = iconv(cd, &buf_iconv, &len_in, &but_out_iconv, &len_out);
		if ((int)r == -1)
			return false;
		*but_out_iconv = *(but_out_iconv + 1) = 0;

		return true;
	}
}
*/


void MakeLower(std::wstring& str) {
	std::transform(str.begin(), str.end(), str.begin(), towlower);
}

void MakeUpper(std::wstring& str) {
	std::transform(str.begin(), str.end(), str.begin(), towupper);
}

void MakeLower(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
}

void MakeUpper(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), toupper);
}


/*
 * STEPMANIA PART SOURCE CODE
 * All rights reserved
 */


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
bool utf8_to_wchar_ec(const RString &s, unsigned &start, wchar_t &ch)
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
void wchar_to_utf8(wchar_t ch, RString &out)
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

wchar_t utf8_get_char(const RString &s)
{
	unsigned start = 0;
	wchar_t ret;
	if (!utf8_to_wchar_ec(s, start, ret))
		return INVALID_CHAR;
	return ret;
}

// Replace invalid sequences in s.
void utf8_sanitize(RString &s)
{
	RString ret;
	for (unsigned start = 0; start < s.size();)
	{
		wchar_t ch;
		if (!utf8_to_wchar_ec(s, start, ch))
			ch = INVALID_CHAR;

		wchar_to_utf8(ch, ret);
	}

	s = ret;
}

bool utf8_is_valid(const RString &s)
{
	for (unsigned start = 0; start < s.size();)
	{
		wchar_t ch;
		if (!utf8_to_wchar_ec(s, start, ch))
			return false;
	}
	return true;
}

/* Windows tends to drop garbage BOM characters at the start of UTF-8 text files.
* Remove them. */
void utf8_remove_bom(RString &sLine)
{
	if (!sLine.compare(0, 3, "\xef\xbb\xbf"))
		sLine.erase(0, 3);
}

static int UnicodeDoUpper(char *p, size_t iLen, const unsigned char pMapping[256])
{
	// Note: this has problems with certain accented characters. -aj
	wchar_t wc = L'\0';
	unsigned iStart = 0;
	if (!utf8_to_wchar(p, iLen, iStart, wc))
		return 1;

	wchar_t iUpper = wc;
	if (wc < 256)
		iUpper = pMapping[wc];
	if (iUpper != wc)
	{
		RString sOut;
		wchar_to_utf8(iUpper, sOut);
		if (sOut.size() == iStart)
			memcpy(p, sOut.data(), sOut.size());
		else
			LOG->Warn(ssprintf("UnicodeDoUpper: invalid character at \"%s\"", RString(p, iLen).c_str()));
	}

	return iStart;
}

/* Fast in-place MakeUpper and MakeLower. This only replaces characters with characters of the same UTF-8
* length, so we never have to move the whole string. This is optimized for strings that have no
* non-ASCII characters. */
void MakeUpper(char *p, size_t iLen)
{
	char *pStart = p;
	char *pEnd = p + iLen;
	while (p < pEnd)
	{
		// Fast path:
		if (likely(!(*p & 0x80)))
		{
			if (unlikely(*p >= 'a' && *p <= 'z'))
				*p += 'A' - 'a';
			++p;
			continue;
		}

		int iRemaining = iLen - (p - pStart);
		p += UnicodeDoUpper(p, iRemaining, g_UpperCase);
	}
}

void MakeLower(char *p, size_t iLen)
{
	char *pStart = p;
	char *pEnd = p + iLen;
	while (p < pEnd)
	{
		// Fast path:
		if (likely(!(*p & 0x80)))
		{
			if (unlikely(*p >= 'A' && *p <= 'Z'))
				*p -= 'A' - 'a';
			++p;
			continue;
		}

		int iRemaining = iLen - (p - pStart);
		p += UnicodeDoUpper(p, iRemaining, g_LowerCase);
	}
}

void UnicodeUpperLower(wchar_t *p, size_t iLen, const unsigned char pMapping[256])
{
	wchar_t *pEnd = p + iLen;
	while (p != pEnd)
	{
		if (*p < 256)
			*p = pMapping[*p];
		++p;
	}
}

void MakeUpper(wchar_t *p, size_t iLen)
{
	UnicodeUpperLower(p, iLen, g_UpperCase);
}

void MakeLower(wchar_t *p, size_t iLen)
{
	UnicodeUpperLower(p, iLen, g_LowerCase);
}


int StringToInt(const RString &sString)
{
	int ret;
	istringstream(sString) >> ret;
	return ret;
}

RString IntToString(const int &iNum)
{
	stringstream ss;
	ss << iNum;
	return ss.str();
}

float StringToFloat(const RString &sString)
{
	float ret = strtof(sString, NULL);

	if (!isfinite(ret))
		ret = 0.0f;
	return ret;
}

bool StringToFloat(const RString &sString, float &fOut)
{
	char *endPtr;

	fOut = strtof(sString, &endPtr);
	return sString.size() && *endPtr == '\0' && isfinite(fOut);
}

RString FloatToString(const float &num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}

wstring RStringToWstring(const RString &s)
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

RString WStringToRString(const wstring &sStr)
{
	RString sRet;

	for (unsigned i = 0; i < sStr.size(); ++i)
		wchar_to_utf8(sStr[i], sRet);

	return sRet;
}

RString WcharToUTF8(wchar_t c)
{
	RString ret;
	wchar_to_utf8(c, ret);
	return ret;
}


template <class S>
static int DelimitorLength(const S &Delimitor)
{
	return Delimitor.size();
}

static int DelimitorLength(char Delimitor)
{
	return 1;
}

static int DelimitorLength(wchar_t Delimitor)
{
	return 1;
}

template <class S, class C>
void do_split(const S &Source, const C Delimitor, vector<S> &AddIt, const bool bIgnoreEmpty)
{
	/* Short-circuit if the source is empty; we want to return an empty vector if
	* the string is empty, even if bIgnoreEmpty is true. */
	if (Source.empty())
		return;

	size_t startpos = 0;

	do {
		size_t pos;
		pos = Source.find(Delimitor, startpos);
		if (pos == Source.npos)
			pos = Source.size();

		if (pos - startpos > 0 || !bIgnoreEmpty)
		{
			/* Optimization: if we're copying the whole string, avoid substr; this
			* allows this copy to be refcounted, which is much faster. */
			if (startpos == 0 && pos - startpos == Source.size())
				AddIt.push_back(Source);
			else
			{
				const S AddRString = Source.substr(startpos, pos - startpos);
				AddIt.push_back(AddRString);
			}
		}

		startpos = pos + DelimitorLength(Delimitor);
	} while (startpos <= Source.size());
}

void split(const RString &sSource, const RString &sDelimitor, vector<RString> &asAddIt, const bool bIgnoreEmpty)
{
	if (sDelimitor.size() == 1)
		do_split(sSource, sDelimitor[0], asAddIt, bIgnoreEmpty);
	else
		do_split(sSource, sDelimitor, asAddIt, bIgnoreEmpty);
}

void split(const wstring &sSource, const wstring &sDelimitor, vector<wstring> &asAddIt, const bool bIgnoreEmpty)
{
	if (sDelimitor.size() == 1)
		do_split(sSource, sDelimitor[0], asAddIt, bIgnoreEmpty);
	else
		do_split(sSource, sDelimitor, asAddIt, bIgnoreEmpty);
}

/* Use:
RString str="a,b,c";
int start = 0, size = -1;
while( 1 )
{
do_split( str, ",", start, size );
if( start == str.size() )
break;
str[start] = 'Q';
}
*/

template <class S>
void do_split(const S &Source, const S &Delimitor, int &begin, int &size, int len, const bool bIgnoreEmpty)
{
	if (size != -1)
	{
		// Start points to the beginning of the last delimiter. Move it up.
		begin += size + Delimitor.size();
		begin = min(begin, len);
	}

	size = 0;

	if (bIgnoreEmpty)
	{
		// Skip delims.
		while (begin + Delimitor.size() < Source.size() &&
			!Source.compare(begin, Delimitor.size(), Delimitor))
			++begin;
	}

	/* Where's the string function to find within a substring?
	* C++ strings apparently are missing that ... */
	size_t pos;
	if (Delimitor.size() == 1)
		pos = Source.find(Delimitor[0], begin);
	else
		pos = Source.find(Delimitor, begin);
	if (pos == Source.npos || (int)pos > len)
		pos = len;
	size = pos - begin;
}

void split(const RString &Source, const RString &Delimitor, int &begin, int &size, int len, const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, len, bIgnoreEmpty);
}

void split(const wstring &Source, const wstring &Delimitor, int &begin, int &size, int len, const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, len, bIgnoreEmpty);
}

void split(const RString &Source, const RString &Delimitor, int &begin, int &size, const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, Source.size(), bIgnoreEmpty);
}

void split(const wstring &Source, const wstring &Delimitor, int &begin, int &size, const bool bIgnoreEmpty)
{
	do_split(Source, Delimitor, begin, size, Source.size(), bIgnoreEmpty);
}


RString join(const RString &sDeliminator, const vector<RString> &sSource)
{
	if (sSource.empty())
		return RString();

	RString sTmp;
	size_t final_size = 0;
	size_t delim_size = sDeliminator.size();
	for (size_t n = 0; n < sSource.size() - 1; ++n)
	{
		final_size += sSource[n].size() + delim_size;
	}
	final_size += sSource.back().size();
	sTmp.reserve(final_size);

	for (unsigned iNum = 0; iNum < sSource.size() - 1; iNum++)
	{
		sTmp += sSource[iNum];
		sTmp += sDeliminator;
	}
	sTmp += sSource.back();
	return sTmp;
}

RString join(const RString &sDelimitor, vector<RString>::const_iterator begin, vector<RString>::const_iterator end)
{
	if (begin == end)
		return RString();

	RString sRet;
	size_t final_size = 0;
	size_t delim_size = sDelimitor.size();
	for (vector<RString>::const_iterator curr = begin; curr != end; ++curr)
	{
		final_size += curr->size();
		if (curr != end)
		{
			final_size += delim_size;
		}
	}
	sRet.reserve(final_size);

	while (begin != end)
	{
		sRet += *begin;
		++begin;
		if (begin != end)
			sRet += sDelimitor;
	}

	return sRet;
}


void TrimLeft(RString &sStr, const char *s)
{
	int n = 0;
	while (n < int(sStr.size()) && strchr(s, sStr[n]))
		n++;

	sStr.erase(sStr.begin(), sStr.begin() + n);
}

void TrimRight(RString &sStr, const char *s)
{
	int n = sStr.size();
	while (n > 0 && strchr(s, sStr[n - 1]))
		n--;

	/* Delete from n to the end. If n == sStr.size(), nothing is deleted;
	* if n == 0, the whole string is erased. */
	sStr.erase(sStr.begin() + n, sStr.end());
}

void Trim(RString &sStr, const char *s)
{
	RString::size_type b = 0, e = sStr.size();
	while (b < e && strchr(s, sStr[b]))
		++b;
	while (b < e && strchr(s, sStr[e - 1]))
		--e;
	sStr.assign(sStr.substr(b, e - b));
}

void StripCrnl(RString &s)
{
	while (s.size() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == '\n'))
		s.erase(s.size() - 1);
}

bool BeginsWith(const RString &sTestThis, const RString &sBeginning)
{
	ASSERT(!sBeginning.empty());
	return sTestThis.compare(0, sBeginning.length(), sBeginning) == 0;
}

bool EndsWith(const RString &sTestThis, const RString &sEnding)
{
	ASSERT(!sEnding.empty());
	if (sTestThis.size() < sEnding.size())
		return false;
	return sTestThis.compare(sTestThis.length() - sEnding.length(), sEnding.length(), sEnding) == 0;
}

RString URLEncode(const RString &sStr)
{
	RString sOutput;
	for (unsigned k = 0; k < sStr.size(); k++)
	{
		char t = sStr[k];
		if (t >= '!' && t <= 'z')
			sOutput += t;
		else
			sOutput += "%" + ssprintf("%02X", t);
	}
	return sOutput;
}

#define ZLIB_CHUNK 16384
// http://www.zlib.net/zpipe.c
bool compress(const char *in, int in_len, char *out, int level) {
	int ret, flush;
	unsigned have;
	z_stream strm;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	int outsize = 0;
	do {
		strm.avail_in = in_len;
		if (strm.avail_in > ZLIB_CHUNK) strm.avail_in = ZLIB_CHUNK;
		in_len -= ZLIB_CHUNK;

		flush = (in_len <= 0) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = (unsigned char*)in;
		in += ZLIB_CHUNK;

		/* run deflate() on input until output buffer not full, finish
		compression if all of source has been read in */
		do {
			strm.avail_out = ZLIB_CHUNK;
			strm.next_out = (unsigned char*)(out + outsize);
			ret = deflate(&strm, flush);    /* no bad return value */
			_ASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = ZLIB_CHUNK - strm.avail_out;
			outsize += have;
		} while (strm.avail_out == 0);
		_ASSERT(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	_ASSERT(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

// http://www.zlib.net/zpipe.c
bool decompress(const char *in, int in_len, char* out) {
	int ret;
	unsigned have;
	z_stream strm;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	int outsize = 0;
	do {
		strm.avail_in = in_len;
		if (strm.avail_in > ZLIB_CHUNK) strm.avail_in = ZLIB_CHUNK;
		in_len -= ZLIB_CHUNK;

		if (strm.avail_in == 0)
			break;
		strm.next_in = (unsigned char*)in;
		in += ZLIB_CHUNK;

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = ZLIB_CHUNK;
			strm.next_out = (unsigned char*)(out + outsize);
			ret = inflate(&strm, Z_NO_FLUSH);
			_ASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}
			have = ZLIB_CHUNK - strm.avail_out;
			outsize += have;
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


/*------ Base64 Encoding Table ------*/
static const char MimeBase64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};
/*------ Base64 Decoding Table ------*/
static int DecodeMimeBase64[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 00-0F */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 10-1F */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  /* 20-2F */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  /* 40-4F */
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  /* 50-5F */
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  /* 60-6F */
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,  /* 70-7F */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 80-8F */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* 90-9F */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* A0-AF */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* B0-BF */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* C0-CF */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* D0-DF */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  /* E0-EF */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1   /* F0-FF */
};

int base64encode(const char *in, int in_len, char **out) {
	unsigned char input[3] = { 0, 0, 0 };
	unsigned char output[4] = { 0, 0, 0, 0 };
	int   index, i, j, size;
	const char *p, *plen;
	plen = in + in_len - 1;
	size = (4 * (in_len / 3)) + (in_len % 3 ? 4 : 0) + 1;
	(*out) = (char*)malloc(size);
	j = 0;
	for (i = 0, p = in; p <= plen; i++, p++) {
		index = i % 3;
		input[index] = *p;
		if (index == 2 || p == plen) {
			output[0] = ((input[0] & 0xFC) >> 2);
			output[1] = ((input[0] & 0x3) << 4) | ((input[1] & 0xF0) >> 4);
			output[2] = ((input[1] & 0xF) << 2) | ((input[2] & 0xC0) >> 6);
			output[3] = (input[2] & 0x3F);
			(*out)[j++] = MimeBase64[output[0]];
			(*out)[j++] = MimeBase64[output[1]];
			(*out)[j++] = index == 0 ? '=' : MimeBase64[output[2]];
			(*out)[j++] = index <  2 ? '=' : MimeBase64[output[3]];
			input[0] = input[1] = input[2] = 0;
		}
	}
	(*out)[j] = '\0';
	return size;
}

int base64decode(const char *in, int in_len, char *out) {
	const char* cp;
	int space_idx = 0, phase;
	int d, prev_d = 0;
	unsigned char c;
	space_idx = 0;
	phase = 0;
	for (cp = in; *cp != '\0'; ++cp) {
		d = DecodeMimeBase64[(int)*cp];
		if (d != -1) {
			switch (phase) {
			case 0:
				++phase;
				break;
			case 1:
				c = ((prev_d << 2) | ((d & 0x30) >> 4));
				if (space_idx < in_len)
					out[space_idx++] = c;
				++phase;
				break;
			case 2:
				c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
				if (space_idx < in_len)
					out[space_idx++] = c;
				++phase;
				break;
			case 3:
				c = (((prev_d & 0x03) << 6) | d);
				if (space_idx < in_len)
					out[space_idx++] = c;
				phase = 0;
				break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}

/*
 * http://people.csail.mit.edu/rivest/Md5.c
 */
#define MD5_CHUNK 1024
void md5(const char *in, int in_size, char *out) {
	MD5_CTX mdContext;
	MD5Init(&mdContext);
	for (int i = 0; i < in_size; i += MD5_CHUNK) {
		int chunksize = in_size - i;
		if (chunksize > MD5_CHUNK) chunksize = MD5_CHUNK;
		MD5Update(&mdContext, in, chunksize);
		in += MD5_CHUNK;
	}
	MD5Final(&mdContext);
	
	// print
	for (int i = 0; i < 16; i++)
		sprintf_s(out + i * 2, 2, "%02x", mdContext.digest[i]);
}






/* IO */

bool GetFileContents(const RString &sPath, RString &sOut, bool bOneLine)
{
	// Don't warn if the file doesn't exist, but do warn if it exists and fails to open.
	FileBasic *file;
	if ((file = FILEMANAGER->LoadFile(sPath)) == 0) {
		return false;
	}

	// todo: figure out how to make this UTF-8 safe. -aj
	RString sData;
	int iGot;
	if (bOneLine)
		iGot = file->ReadLine(sData);
	else
		iGot = file->Read(sData, file->GetSize());

	if (iGot == -1)
	{
		LOG->Warn("File(%s) exists but cannot read.", sPath.c_str());
		delete file;
		return false;
	}

	if (bOneLine)
		StripCrnl(sData);

	sOut = sData;
	delete file;
	return true;
}

/* read per line */
bool GetFileContents(const RString &sFile, vector<RString> &asOut)
{
	FileBasic *file;
	if ((file = FILEMANAGER->LoadFile(sFile)) ==0)
	{
		LOG->Warn("GetFileContents(%s): Cannot open file.", sFile.c_str());
		return false;
	}

	RString sLine;
	while (file->ReadLine(sLine))
		asOut.push_back(sLine);
	return true;
}

bool WriteFileContents(const RString &sPath, const char *p, size_t len) {
	File f;
	if (!f.Open(sPath, "wb"))
		return false;
	f.Write(p, len);
	f.Close();
}

bool WriteFileContents(const RString &sPath, RString &sOut) {
	return WriteFileContents(sPath, sOut.c_str(), sOut.size());
}



RString ssprintf(const char *fmt, ...)
{
	va_list	va;
	va_start(va, fmt);
	return vssprintf(fmt, va);
}

#define FMT_BLOCK_SIZE		2048 // # of bytes to increment per try

RString vssprintf(const char *szFormat, va_list argList)
{
	RString sStr;

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

RString GetHash(const RString &sPath) {
	File f;
	f.Open(sPath, "rb");
	RString hash = f.GetMD5Hash();
	f.Close();
	return hash;
}


uint32_t MakeRGBAInt(const RString& s) {
	const char* s_ = s.c_str();
	if (*s_ && s_[0] == '#') s_++;
	std::string as, rs, gs, bs;
	int a = 0, r = 0, g = 0, b = 0;
	if (s.size() == 8) {
		as = std::string(s_, 0, 2);
		rs = std::string(s_, 2, 2);
		gs = std::string(s_, 4, 2);
		bs = std::string(s_, 6, 2);
	}
	else {
		rs = std::string(s_, 0, 2);
		gs = std::string(s_, 2, 2);
		bs = std::string(s_, 4, 2);
	}
	std::istringstream(as) >> std::hex >> a;
	std::istringstream(rs) >> std::hex >> r;
	std::istringstream(gs) >> std::hex >> g;
	std::istringstream(bs) >> std::hex >> b;

	return (r << 24) | (g << 16) | (b << 8) | a;
}