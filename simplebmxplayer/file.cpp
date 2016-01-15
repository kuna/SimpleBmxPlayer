#include "file.h"
#include "globalresources.h"
#include <sys/stat.h>
#include "util.h"

#define READLINE_MAX 10240
#define READALL_MAX 10240000	// about 10mib

File::File() : fp(0) {}
File::~File() {
	Close();
}

bool File::Open(const char *path, const char* mode) {
	/* if file is already open then close it */
	if (fp) Close();

	int e = fopen_s(&fp, path, mode);
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
	char basepath[1024];

	/* private */
	bool CheckIsAbsolutePath(const char *path) {
		return (path[0] != 0 && (path[0] == '/' || path[1] == ':'));
	}

	void SetBasePath(const char *path) {
		ASSERT(CheckIsAbsolutePath(path));
		strcpy(basepath, path);
		if (path[strlen(path) - 1] != '/' || path[strlen(path) - 1] != '\\') {
			if (strchr(path, '/'))
				strcat(basepath, "/");
			else
				strcat(basepath, "\\");
		}
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
				RString key = path.substr(p + 1, p2 - p);
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
		ASSERT(CheckIsAbsolutePath(basepath));
		path = basepath + path;
	}
}