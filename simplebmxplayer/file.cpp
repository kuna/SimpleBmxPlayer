#include "file.h"
#include "globalresources.h"
#include <sys/stat.h>
#include "util.h"
#include "md5.h"

#define READLINE_MAX 10240
#define READALL_MAX 10240000	// about 10mib

File::File() : fp(0) {}
File::~File() {
	Close();
}

bool File::Open(const char *path, const char* mode) {
	/* if file is already open then close it */
	if (fp) Close();
	int e;
#if _WIN32
	wchar_t path_w[1024], mode_w[32];
	ENCODING::utf8_to_wchar(path, path_w, 1024);
	ENCODING::utf8_to_wchar(mode, mode_w, 32);
	e = _wfopen_s(&fp, path_w, mode_w);
#else
	e = fopen_s(&fp, path, mode);
#endif
	if (e != 0)
		return false;

	fname = path;
	return true;
}

void File::Close() {
	if (fp) {
		fclose(fp);
		fp = 0;
		fname.clear();
	}
}

int File::ReadLine(RString &str) {
	char *l = new char[READLINE_MAX];
	char *e = fgets(l, READLINE_MAX, fp);
	int r;
	if (!e)
		r = -1;
	else
		r = e - l;
	str = l;
	delete l;
	return r;
}

int File::Read(RString &str, size_t size) {
	char *p = new char[size];
	size_t r = fread(p, 1, size, fp);
	str = p;
	delete p;
	return r;
}

int File::ReadAll(char *p) {
	return fread(p, 1, READALL_MAX, fp);
}

int File::ReadAll(RString &str) {
	size_t s = GetFileSize();
	char *p = new char[s];
	int r = ReadAll(p);
	str = p;
	delete p;
	return r;
}

void File::Reset() {
	fseek(fp, 0, SEEK_SET);
}

void File::Seek(size_t pos) {
	fseek(fp, pos, SEEK_SET);
}

bool File::IsEOF() {
	return feof(fp);
}

size_t File::GetFileSize() {
	int fd = fileno(fp);
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

namespace FileHelper {
	std::vector<RString> basepath_stack;

	/* private */
	bool CheckIsAbsolutePath(const char *path) {
		return (path[0] != 0 && (path[0] == '/' || path[1] == ':'));
	}

	/* mount */
	void PushBasePath(const char *path) {
		ASSERT(CheckIsAbsolutePath(path));
		char basepath[1024];
		strcpy(basepath, path);
		if (path[strlen(path) - 1] != '/' || path[strlen(path) - 1] != '\\') {
			if (strchr(path, '/'))
				strcat(basepath, "/");
			else
				strcat(basepath, "\\");
		}
		basepath_stack.push_back(basepath);
	}

	/* unmount */
	void PopBasePath() {
		basepath_stack.pop_back();
	}

	RString& GetBasePath() {
		ASSERT(basepath_stack.size() > 0);
		return basepath_stack.back();
	}

	void ConvertPathToAbsolute(RString &path) {
		// replace some env into valid string (refers STRINGPOOL)
		int p = 0, p2 = 0;
		while (p != RString::npos) {
			p = path.find("$(", p);
			if (p == RString::npos)
				break;
			p2 = path.find(")", p);
			if (p2 != RString::npos) {
				RString key = path.substr(p + 2, p2 - p - 2);
				RString val = "";
				if (STRPOOL->IsExists(key))
					val = *STRPOOL->Get(key);
				path = path.substr(0, p) + val + path.substr(p2 + 1);
				p = p2 + 1;
				if (p >= path.size())
					break;
			}
			else {
				break;
			}
		}
		// no empty path
		if (path == "")
			return;
		if (path[0] == '/' || path.substr(1, 2) == ":\\" || path.substr(1, 2) == ":/")
			return;		// it's already absolute path, dont touch it.
		// if relative, then translate it into absolute
		if (path.substr(0, 2) == "./" || path.substr(0, 2) == ".\\")
			path = path.substr(2);
		path = GetBasePath() + path;
	}

#ifdef _WIN32
#include <Windows.h>
#endif
	void GetFileList(const char* folderpath, std::vector<RString>& out, bool getfileonly) {
		RString directory = folderpath;
		ConvertPathToAbsolute(directory);

#ifdef _WIN32
		HANDLE dir;
		WIN32_FIND_DATA file_data;

		wchar_t *directory_w = new wchar_t[1024];
		ENCODING::utf8_to_wchar(directory.c_str(), directory_w, 1024);
		if ((dir = FindFirstFileW((wstring(directory_w) + L"/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
			return; /* No files found */

		do {
			const wstring file_name = file_data.cFileName;
			const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (file_name[0] == '.')
				continue;

			if (getfileonly && is_directory)
				continue;

			const wstring full_file_name = wstring(directory_w) + L"/" + file_name;
			char *full_file_name_utf8 = new char[2048];
			ENCODING::wchar_to_utf8(full_file_name.c_str(), full_file_name_utf8, 2048);
			out.push_back(full_file_name_utf8);
			delete full_file_name_utf8;
		} while (FindNextFile(dir, &file_data));

		delete directory_w;
		FindClose(dir);
#else
		DIR *dir;
		class dirent *ent;
		class stat st;

		dir = opendir(directory);
		while ((ent = readdir(dir)) != NULL) {
			const string file_name = ent->d_name;
			const string full_file_name = directory + "/" + file_name;

			if (file_name[0] == '.')
				continue;

			if (stat(full_file_name.c_str(), &st) == -1)
				continue;

			const bool is_directory = (st.st_mode & S_IFDIR) != 0;

			if (getfileonly && is_directory)
				continue;

			out.push_back(full_file_name);
		}
		closedir(dir);
#endif
	}

	void FilterFileList(const char *extfilters, std::vector<RString>& filelist) {
		std::vector<RString> filters;
		split(extfilters, ";", filters);

		for (auto filepath = filelist.begin(); filepath != filelist.end(); ) {
			for (auto ext = filters.begin(); ext != filters.end(); ++ext) {
				if (EndsWith(*filepath, *ext)) {
					filepath = filelist.erase(filepath);
					continue;
				}
			}
			++filepath;
		}
	}
}