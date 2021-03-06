#include "file.h"
#include "globalresources.h"
#include <zzip/lib.h>		// zzip library
#include <sys/stat.h>
#include "util.h"
#include "md5.h"

#define READLINE_MAX 10240
#define READALL_MAX 10240000	// about 10mib

#define MD5HASH_BLOCKSIZE 1024
RString FileBasic::GetMD5Hash() {
	// md5
	MD5_CTX mdContext;
	char* data = new char[MD5HASH_BLOCKSIZE];
	MD5Init(&mdContext);
	int bytes;
	Seek(0);
	while ((bytes = Read(data, MD5HASH_BLOCKSIZE)) != 0) {
		MD5Update(&mdContext, data, bytes);
	}
	MD5Final(&mdContext);
	// digest
	char digest[40];
	digest[32] = 0;
	char *digest_p = digest;
	for (int i = 0; i < 16; i++) {
		sprintf(digest_p, "%02x", mdContext.digest[i]);
		digest_p += 2;
	}
	// release & return
	delete data;
	return digest;
}






/*
 * class File
 * general IO class
 */

File::File() : fp(0) {}
File::File(const char* filename, const char* mode) { fp = 0; Open(filename, mode); }
File::~File() {
	Close();
}

bool File::Open(const char *path, const char* mode) {
	/* if file is already open then close it */
	if (fp) Close();
	int e;
#if _WIN32
	fpath_w = RStringToWstring(path);
	fmode_w = RStringToWstring(mode);
	e = _wfopen_s(&fp, fpath_w.c_str(), fmode_w.c_str());
#else
	e = fopen_s(&fp, path, mode);
#endif
	if (e != 0)
		return false;

	fpath = path;
	fmode = mode;
	return true;
}

void File::Close() {
	if (fp) {
		fclose(fp);
		fp = 0;
		fpath.clear();
		fmode.clear();
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

int File::Read(char *p, size_t size) {
	size_t r = fread(p, 1, size, fp);
	return r;
}

int File::ReadAll(char *p) {
	return fread(p, 1, READALL_MAX, fp);
}

int File::ReadAll(RString &str) {
	size_t s = GetSize();
	char *p = new char[s];
	int r = ReadAll(p);
	str = p;
	delete p;
	return r;
}

int File::Write(const RString &in) {
	return Write(in.c_str(), in.size());
}

int File::Write(const char* in, int len) {
	return fwrite(in, 1, len, fp);
}

SDL_RWops* File::GetSDLRW() {
#ifdef _WIN32
	// since windows can't share fp, create new fp
	return SDL_RWFromFile(fpath.c_str(), fmode.c_str());
#else
	return SDL_RWFromFP(fp, SDL_FALSE);
#endif
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

size_t File::GetSize() {
	int fd = fileno(fp);
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}





/*
 * class FileMemory
 * file object for memory
 * (compatible with SDL_)
 */

FileMemory::FileMemory() { m_pos = 0; }

FileMemory::FileMemory(const char* data, size_t size)
: FileMemory() {
	Set(data, size);
}

void FileMemory::Set(const char* data, size_t size) {
	m_data.resize(size);
	for (int i = 0; i < size; i++)
		m_data[i] = data[i];
}

void FileMemory::Close() {
	m_data.clear();
	m_pos = 0;
}

size_t FileMemory::GetSize() {
	return m_data.size();
}

int FileMemory::Read(RString &str, size_t size) {
	int totsize = GetSize();
	if (totsize == 0)
		return 0;
	str.resize(size);
	for (int i = 0; i < size; i++) {
		if (m_pos >= totsize) {
			m_pos = totsize - 1;
			return i + 1;
		}
		str[i] = m_data[m_pos++];
	}
	return totsize;
}

int FileMemory::Read(char *p, size_t size) {
	int totsize = GetSize();
	if (totsize == 0)
		return 0;
	int i = 0;
	for (; i < size; i++) {
		if (m_pos >= totsize) {
			break;
		}
		p[i] = m_data[m_pos++];
	}
	return i;
}

int FileMemory::ReadLine(RString &str) {
	int totsize = GetSize();
	if (totsize == 0)
		return 0;
	str = "";
	int read = 0;
	while (m_pos < totsize) {
		str.push_back(m_data[m_pos++]);
		read++;
		if (m_pos >= totsize || m_data[m_pos] == '\n') break;
	}
	return read;
}

int FileMemory::ReadAll(RString &str) {
	str.resize(m_data.size());
	for (int i = 0; i < m_data.size(); i++) {
		str[i] = m_data[i];
	}
	return m_data.size();
}

int FileMemory::ReadAll(char *p) {
	for (int i = 0; i < m_data.size(); i++) {
		p[i] = m_data[i];
	}
	return m_data.size();
}

int FileMemory::Write(const RString& in) {
	return Write(in.c_str(), in.size());
}

int FileMemory::Write(const char* in, int len) {
	if (m_pos + len > m_data.size()) {
		m_data.resize(m_pos + len);
	}
	for (int i = 0; i < len; i++)
		m_data[m_pos++] = *in++;
	return len;
}

bool FileMemory::IsEOF() { return GetSize() < m_pos; }

void FileMemory::Reset() {
	m_pos = 0;
}

void FileMemory::Seek(size_t pos) {
	m_pos = pos;
}

SDL_RWops* FileMemory::GetSDLRW() {
	return SDL_RWFromConstMem(&m_data.front(), m_data.size());
}






/*
 * helper for file class
 */

namespace FileHelper {
	/* used for checking current mounting */
	struct mount_status_ {
		RString path_;
		bool iszipfile_;
		ZZIP_DIR *zipdir_;
	};

	/* all mount status are stored in here */
	std::vector<mount_status_> basepath_stack;

	/* private */
	bool CheckIsAbsolutePath(const char *path) {
		return (path[0] != 0 && (path[0] == '/' || path[1] == ':'));
	}

	/* mount (MUST insert absolute path) */
	void PushBasePath(const char *path) {
		ASSERT(CheckIsAbsolutePath(path));
		RString basepath = path;

		mount_status_ stat_;
		if (EndsWith(basepath, ".zip")) {
			stat_.zipdir_ = zzip_opendir(basepath);
			ASSERT(stat_.zipdir_ != 0);
			stat_.iszipfile_ = true;
		}
		else {
			stat_.iszipfile_ = false;
		}

		// (directory should end with `/`)
		if (basepath[strlen(basepath) - 1] != '/' && basepath[strlen(basepath) - 1] != '\\') {
			basepath += "/";
		}

		stat_.path_ = basepath;
		basepath_stack.push_back(stat_);
	}

	/* unmount */
	void PopBasePath() {
		mount_status_ stat_ = basepath_stack.back();
		if (stat_.iszipfile_) {
			zzip_dir_close(stat_.zipdir_);
		}
		basepath_stack.pop_back();
	}

	/*
	* COMMENT: bug can occur
	* if basepath zipfile & target normalfile /
	*    basepath normalfile & target zipfile.
	* use it at own risk..
	* COMMENT: path should be relative. (suggest)
	*          if not relative, then this code will convert path into relative one.
	*/
	bool LoadFile(const char *relpath, FileBasic **f) {
		RString path_ = relpath;
		mount_status_ stat_ = basepath_stack.back();
		if (stat_.iszipfile_) {
			if (CheckIsAbsolutePath(path_)) {
				path_ = get_filename(path_);
			}
			// memory file return
			ZZIP_FILE *zfp = zzip_file_open(stat_.zipdir_, path_, 0);
			if (!zfp) return false;
			ZZIP_STAT zstat;
			zzip_file_stat(zfp, &zstat);
			int orgsize = zstat.st_size;
			char *buf_tmp_ = new char[orgsize];
			zzip_file_read(zfp, buf_tmp_, orgsize);
			*f = new FileMemory(buf_tmp_, orgsize);
			delete[] buf_tmp_;
			zzip_file_close(zfp);
		}
		else {
			// normal file return
			ConvertPathToAbsolute(path_);
			if (!IsFile(path_)) return false;
			*f = new File(path_, "rb");
		}
		return true;
	}

	bool CurrentDirectoryIsZipFile() {
		mount_status_ stat_ = basepath_stack.back();
		return stat_.iszipfile_;
	}

	/*
	* get current directory's file date
	*/
	bool GetDirDate(const char* relpath) {
		return false;
	}

	/*
	 * get current directory's file list (include dir)
	 */
	void GetFileList(std::vector<RString>& list) {
		mount_status_ stat_ = basepath_stack.back();
		if (stat_.iszipfile_) {
			ZZIP_DIRENT dirent;
			while (zzip_dir_read(stat_.zipdir_, &dirent)) {
				list.push_back(dirent.d_name);
			}
		}
		else {
			GetFileList(stat_.path_, list);
		}
	}

	RString& GetBasePath() {
		ASSERT(basepath_stack.size() > 0);
		return basepath_stack.back().path_;
	}

	RString& GetSystemPath() {
		ASSERT(basepath_stack.size() > 0);
		return basepath_stack.front().path_;
	}

	void ReplacePathEnv(RString& path) {
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
	}

	void ConvertPathToAbsolute(RString &path, RString &base) {
		// no empty path
		if (path == "")
			return;
		ReplacePathEnv(path);
		if (path[0] == '/' || path.substr(1, 2) == ":\\" || path.substr(1, 2) == ":/")
			return;		// it's already absolute path, dont touch it.
		// if relative, then translate it into absolute
		if (path.substr(0, 2) == "./" || path.substr(0, 2) == ".\\")
			path = path.substr(2);
		path = base + path;
	}

	void ConvertPathToAbsolute(RString &path) {
		return ConvertPathToAbsolute(path, GetBasePath());
	}

	void ConvertPathToSystem(RString &path) {
		return ConvertPathToAbsolute(path, GetSystemPath());
	}

	bool GetAnyAvailableFilePath(RString &path) {
		if (IsFile(path)) return true;
		ConvertPathToAbsolute(path);
		if (IsFile(path)) return true;
		int p = path.find_last_of("./\\");
		if (p == std::string::npos) return false;
		RString ext = path.substr(p);
		RString folder = GetParentDirectory(path);
		std::vector<RString> filelist;
		GetFileList(folder, filelist);
		FilterFileList(ext, filelist);
		if (filelist.size() > 0) {
			path = filelist[rand() % filelist.size()];
			return true;
		}
		return false;
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

		std::wstring directory_w = RStringToWstring(directory);
		if ((dir = FindFirstFileW((directory_w + L"/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
			return; /* No files found */

		do {
			const wstring file_name = file_data.cFileName;
			const bool is_directory = 
				(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
				file_name.substr(file_name.size()-4) == L".zip";

			if (file_name[0] == '.')
				continue;

			if (getfileonly && is_directory)
				continue;

			const wstring full_file_name = directory_w + L"/" + file_name;
			RString full_file_name_utf8 = WStringToRString(full_file_name);
			out.push_back(full_file_name_utf8);
		} while (FindNextFile(dir, &file_data));

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
			bool update = false;
			for (auto ext = filters.begin(); ext != filters.end(); ++ext) {
				if (EndsWith(*filepath, *ext)) {
					update = true;
					break;
				}
			}
			if (!update) filepath = filelist.erase(filepath);
			else ++filepath;
		}
	}

	/*
	 * uses relative file path (supports zip)
	 */
	bool IsRelativeFile(const RString& path) {
		if (EndsWith(path, ".zip")) return false;
		RString relpath = path;
		ConvertPathToAbsolute(relpath);
		if (IsFile(relpath)) return true;
		//
		// still not exists, then search archive if it does.
		//
		if (CurrentDirectoryIsZipFile()) {
			std::vector<RString> files;
			GetFileList(files);
			for (int i = 0; i < files.size(); i++) {
				if (files[i] == path) return true;
			}
		}

		return false;
	}

	bool IsFile(const RString& path) {
		if (EndsWith(path, ".zip")) return false;
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(path);
		struct _stat64i32 s;
		if (_wstat(path_w.c_str(), &s) != 0) return false;
		if (s.st_mode & S_IFREG) return true;
		else return false;
#else
		struct stat s;
		if (stat(path.c_str(), &s) != 0) return false;
		if (s.st_mode & S_IFREG) return true;
		else return false;
#endif
	}

	bool IsFolder(const RString& path) {
		if (EndsWith(path, ".zip")) return true;
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(path);
		struct _stat64i32 s;
		if (_wstat(path_w.c_str(), &s) != 0) return false;
		if (s.st_mode & S_IFDIR) return true;
		else return false;
#else
		struct stat s;
		if (stat(path.c_str(), &s) != 0) return false;
		if (s.st_mode & S_IFDIR) return true;
		else return false;
#endif
	}

	// private
	bool _create_directory(const char* filepath) {
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(filepath);
		return (_wmkdir(path_w.c_str()) == 0);
#else
		return (mkdir(filepath) == 0);
#endif
	}

	bool CreateFolder(const RString& path) {
		// if current directory is not exist
		// then get parent directory
		// and check it recursively
		// after that, create own.
		if (IsFolder(path))
			return true;
		if (IsFile(path))
			return false;
		if (!_create_directory(path)) {
			RString parent = GetParentDirectory(path);
			if (NOT(CreateFolder(parent))) {
				return false;
			}
			return _create_directory(path);
		}
		else return true;
	}

	RString GetParentDirectory(const RString& path) {
		return path.substr(0, path.find_last_of("/\\"));
	}
}