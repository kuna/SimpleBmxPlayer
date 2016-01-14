#include "file.h"
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
