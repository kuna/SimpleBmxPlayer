#include "file.h"
#include "Pool.h"
#include "logger.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"
#include <sys/stat.h>
#include "util.h"
#include "md5.h"

#ifdef _WIN32
#include <Windows.h>
#endif

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
	RString abspath = FILEMANAGER->GetAbsolutePath(path);
	int e;
#if _WIN32
	fpath_w = RStringToWstring(abspath);
	fmode_w = RStringToWstring(mode);
	e = _wfopen_s(&fp, fpath_w.c_str(), fmode_w.c_str());
#else
	e = fopen_s(&fp, abspath, mode);
#endif
	if (e != 0)
		return false;

	fpath = abspath;
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

int File::WriteToFile(const char* path) {
	RString s;
	Reset();
	ReadAll(s);
	int r = WriteFileContents(path, s);
	return r;
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

int FileMemory::WriteToFile(const char* path) {
	RString s;
	Reset();
	ReadAll(s);
	int r = WriteFileContents(path, s);
	return r;
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





void FileManagerBasic::PushBasePath(const RString& path) {
	// convert path to directory before push
	m_Basepath.push_back(GetDirectory(path));
}

void FileManagerBasic::PopBasePath() {
	m_Basepath.pop_back();
}

RString FileManagerBasic::GetBasePath() {
	return m_Basepath.back();
}

void FileManagerBasic::SetFileFilter(const RString& filter) {
	split(filter, ";", m_Filterext);
}

/* info */
bool FileManagerBasic::GetFileInfo(const RString& path, FileInfo &info) {
	RString abspath = GetAbsolutePath(path);
	// if there's no information in real file system ...
	if (!FileHelper::GetFileInfo(abspath, info)) {
		// search for mounted file
		RString dirpath = GetDirectory(abspath);
		RString filename = abspath.substr(dirpath.size() + 1);
		for (auto it = m_Mount.begin(); it != m_Mount.end(); ++it) {
			if (stricmp(it->first, dirpath) == 0) {
				for (auto fit = it->second.files.begin(); fit != it->second.files.end(); ++fit) {
					if (stricmp(fit->filename, filename) == 0) {
						info = *fit;
						return true;
					}
				}
				break;
			}
		}
	}
	return false;
}

namespace {
	bool IsFileFiltered(const RString& filename, std::vector<RString>& filters) {
		for (auto ext = filters.begin(); ext != filters.end(); ++ext) {
			if (EndsWith(filename, *ext)) {
				return true;
			}
		}
		return false;
	}
}

void FileManagerBasic::GetDirectoryFileList(const RString& dirpath, std::vector<RString> files) {
	std::vector<FileInfo> _tmp;
	GetDirectoryFileInfo(dirpath, _tmp);
	for (auto it = _tmp.begin(); it != _tmp.end(); it++)
		files.push_back(it->filename);
}

// TODO: filter file list
void FileManagerBasic::GetDirectoryFileInfo(const RString& dirpath, std::vector<FileInfo> files) {
	// convert to absolute, valid directory path
	RString directory = GetDirectory(dirpath);

	// scan for mount path first
	std::map<RString, MountInfo>::iterator iter;
	if (m_SearchMountPath && (iter = m_Mount.find(directory)) != m_Mount.end()) {
		files = iter->second.files;
		return;
	}

	// scan for base path
	directory = GetAbsolutePath(directory);

#ifdef _WIN32
	HANDLE dir;
	WIN32_FIND_DATA file_data;

	std::wstring directory_w = RStringToWstring(directory);
	if ((dir = FindFirstFileW((directory_w + L"/*").c_str(), &file_data)) != INVALID_HANDLE_VALUE) {
		do {
			const wstring file_name = file_data.cFileName;

			// ignore `.`, `..`
			if (file_name[0] == '.')
				continue;

			RString fn_utf8 = WStringToRString(file_name);

			// check for file filter
			if (IsFileFiltered(fn_utf8, m_Filterext))
				continue;

			// fill file information
			const bool is_directory =
				(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			FileInfo finfo;
			finfo.type = is_directory ? FILETYPE::TYPE_FOLDER : FILETYPE::TYPE_FILE;
			finfo.data = 0;
			finfo.createtime = file_data.ftCreationTime.dwHighDateTime;
			finfo.updatetime = file_data.ftLastWriteTime.dwHighDateTime;
			finfo.size = file_data.nFileSizeHigh;

			files.push_back(finfo);
		} while (FindNextFile(dir, &file_data));

		FindClose(dir);
	}
#else
	DIR *dir;
	class dirent *ent;
	class stat st;

	dir = opendir(directory);
	while ((ent = readdir(dir)) != NULL) {
		const string file_name = ent->d_name;
		//const string full_file_name = directory + "/" + file_name;

		// ignore `.`, `..`
		if (file_name[0] == '.')
			continue;

		// ??
		if (stat(full_file_name.c_str(), &st) == -1)
			continue;

		// check for file filter
		if (IsFileFiltered(file_name, m_Filterext))
			continue;

		// fill file information
		const bool is_directory = (st.st_mode & S_IFDIR) != 0;
		struct stat st;
		stat(ent->d_name, &st);
		FileInfo finfo;
		finfo.type = is_directory ? FILETYPE::TYPE_FOLDER : FILETYPE::TYPE_FILE;
		finfo.data = 0;
		finfo.createtime = st->st_ctime;
		finfo.updatetime = st->st_mtime;
		finfo.size = ent->d_size;

		files.push_back(finfo);
	}
	closedir(dir);
#endif

}

bool FileManagerBasic::IsMountedFile(const RString& path) {
	RString abspath = GetAbsolutePath(path);
	RString dirpath = GetDirectory(abspath);
	RString filename = abspath.substr(dirpath.size() + 1);
	for (auto it = m_Mount.begin(); it != m_Mount.end(); ++it) {
		if (stricmp(it->first, dirpath) == 0) {
			for (auto fit = it->second.files.begin(); fit != it->second.files.end(); ++fit) {
				if (stricmp(fit->filename, filename) == 0) {
					return true;
				}
			}
			break;
		}
	}
	return false;
}

bool FileManagerBasic::IsFile(const RString& path) {
	if (FileHelper::IsFile(path))
		return FILETYPE::TYPE_FILE;
	else {
		// search for mounted file
		FileInfo finfo;
		if (GetFileInfo(path, finfo))
			return finfo.type == FILETYPE::TYPE_FILE;
		// cannot found
		return false;
	}
}

bool FileManagerBasic::IsDirectory(const RString& path) {
	if (FileHelper::IsDirectory(path))
		return FILETYPE::TYPE_FOLDER;
	else {
		// search for mount list
		RString abspath = GetAbsolutePath(path);
		RString dirpath = GetDirectory(abspath);
		for (auto it = m_Mount.begin(); it != m_Mount.end(); ++it) {
			if (stricmp(it->first, dirpath) == 0) {
				return true;
			}
		}

#if 0
		FileInfo finfo;
		if (GetFileInfo(path, finfo))
			return finfo.type == FILETYPE::TYPE_FOLDER;
#endif

		return false;
	}
}

/* these functions doesn't gurantee file existing */
bool FileManagerBasic::IsAbsolutePath(const RString& path) {
	return (path[0] != 0 && (path[0] == '/' || path[1] == ':'));
}

namespace {
	// remove ./ or .\
	//
	inline void TidyPath(RString& path) {
		if (path.size() >= 2 && path.substr(0, 2) == "./" || path.substr(0, 2) == ".\\")
			path = path.substr(2);
	}
}

RString FileManagerBasic::GetAbsolutePath(const RString& path) {
	// no empty path
	if (path.size() == 0)
		return "";
	if (IsAbsolutePath(path))
		return path;		// it's already absolute path, dont touch it.

	// start converting
	RString newpath = path;
	TidyPath(newpath);

	// make it absolute!
	// - this always considers basepath, not mounting.
	newpath = GetBasePath() + newpath;
	return newpath;
}

bool FileManagerBasic::GetExistingAbsolutePath(const RString& path, RString& out) {
	// no empty path
	if (path.size() == 0)
		return "";
	if (IsAbsolutePath(path))
		return path;		// it's already absolute path, dont touch it.

	// start converting
	RString newpath = path;
	TidyPath(newpath);

	// only difference: this searches IO / mountpath for all stacked base paths.
	for (int i = m_Basepath.size() - 1; i >= 0; i--) {
		RString path2 = m_Basepath[i] + newpath;
		if (m_Mount.find(path2) != m_Mount.end() || IsFile(path2) || IsDirectory(path2)) {
			out = path2;
			return true;
		}
	}

	// cannot find available/existing path
	return false;
}

RString FileManagerBasic::GetRelativePath(const RString& path) {
	return GetRelativePath(path, GetBasePath());
}

// Absolutepath from Relativepath
// remove first part of path if it exists in basepath
RString FileManagerBasic::GetRelativePath(const RString& path, const RString& basepath) {
	// TODO: process .. if necessary
	RString relpath = path;
	TidyPath(relpath);
	if (!IsAbsolutePath(relpath)) relpath = GetAbsolutePath(relpath);
	if (BeginsWith(relpath, basepath)) relpath = relpath.substr(basepath.size() + 1);
	return relpath;
}

RString FileManagerBasic::GetDirectory(const RString& path) {
	// (directory should end with `/`)
	ASSERT(path.size() != 0);	// don't allow empty path
	int pos = path.find_last_of("/\\");
	if (pos == path.size() - 1) return path;	// already directory
	else return path.substr(0, pos);
}

RString FileManagerBasic::GetParentDirectory(const RString& path) {
	RString dir = GetDirectory(path);
	dir.pop_back();
	return GetDirectory(dir);
}

/* mounting */
bool FileManagerBasic::Mount(const RString& path) {
	// if not directory, then don't mount.
	if (!IsDirectory(path)) return false;

	// do mount!
	RString abspath = GetAbsolutePath(GetDirectory(path));
	// ignore if already mounted
	if (m_Mount.find(abspath) != m_Mount.end()) {
		MountInfo *minfo = &m_Mount[abspath];
		GetDirectoryFileInfo(abspath, minfo->files);
		minfo->handle = 0;
		minfo->mountpath = abspath;
		GetFileInfo(abspath, minfo->mountinfo);
	}
}

void FileManagerBasic::UnMount(const RString& path) {
	// CAUTION: handle/data isn't released by itself, so you should overwrite this function to process that.
	RString abspath = GetAbsolutePath(GetDirectory(path));
	m_Mount.erase(abspath);
}

/* I/O */
FileBasic* FileManagerBasic::LoadFile(const RString& path) {
	// attempt to search normal file first
	RString abspath = GetAbsolutePath(path);
	if (IsFile(abspath)) {
		return new File(abspath, "rb");
	}

	// check mounted file
	RString dirpath = GetDirectory(path);
	auto iter = m_Mount.find(dirpath);
	if (iter != m_Mount.end()) {
		RString filename = path.substr(dirpath.size() + 1);
		for (int i = 0; i < iter->second.files.size(); i++) {
			FileInfo& finfo = iter->second.files[i];
			if (stricmp(finfo.filename, filename) == 0 && finfo.data) {
				FileMemory *f = new FileMemory((char*)finfo.data, finfo.size);
				return f;
			}
		}
	}

	// cannot found
	return 0;
}

bool FileManagerBasic::ReadAllFile(const RString& path, char **p, int *len) {
	FileBasic* f = LoadFile(path);
	if (!f)
		return false;

	*len = f->GetSize();
	f->Reset();
	*p = (char*)malloc(*len);
	f->ReadAll(*p);
	f->Close();
	return true;
}

/*
 * In case of symbol collision
 */
#ifdef CreateFile
#undef CreateFile
#endif
#ifdef CreateDirectory
#undef CreateDirectory
#endif

bool FileManagerBasic::CreateFile(const RString& path) {
	// TODO ...?
	return false;
}

bool FileManagerBasic::CreateDirectory(const RString& path) {
	return FileHelper::CreateDirectory(GetAbsolutePath(GetDirectory(path)));
}

FileManagerBasic::~FileManagerBasic() {
	// if mounted file exists, then release them all automatically.
	// COMMENT: you should overwrite this function if you changed UnMount() behaviour.
	for (auto it = m_Mount.begin(); it != m_Mount.end(); ++it) {
		UnMount(it->first);
	}
	m_Mount.clear();
}






namespace {
	int archive_initialize_count = 0;
}

FileManager::FileManager() {
	if (archive_initialize_count == 0) {
		// nothing to do?
	}
	archive_initialize_count++;
}

FileManager::~FileManager() {
	archive_initialize_count--;
	if (archive_initialize_count == 0) {
		//
	}
}

void FileManager::PushBasePath(const RString& path) {
	// we support archive file as directory.
	// CAUTION: it's not auto-mounted, so you should do it by your own.
	RString newpath = path;
	if (FileHelper::IsArchiveFileName(newpath))
		newpath = path + "/";
	FileManagerBasic::PushBasePath(path);
}

#define ITER_ARCHIVE(a, aent)\
	for (int r = archive_read_next_header(a, &aent);\
		r != ARCHIVE_EOF;\
		r = archive_read_next_header(a, &aent))
namespace {
	int OpenArchive(archive *a, const RString &abspath) {
		//archive_read_support_format_7zip(a);
		//archive_read_support_format_rar(a);
		//archive_read_support_format_zip(a);
		archive_read_support_format_all(a);
		archive_read_support_compression_all(a);

#ifdef _WIN32
		std::wstring wpath = RStringToWstring(abspath);
		return archive_read_open_filename_w(a, wpath.c_str(), 10240);
#else
		return archive_read_open_filename(a, abspath, 10240);
#endif
	}

	void CloseArchive(archive *a) {
		archive_read_close(a);
		archive_read_finish(a);
	}
}

bool FileManager::Mount(const RString& path) {
	// if archive filename?
	if (FileHelper::IsArchiveFileName(path)) {
		RString abspath = GetAbsolutePath(path + "/");

		// opens archive file
		struct archive *a;
		a = archive_read_new();
		if (OpenArchive(a, abspath) == 0)
		{
			// create structure
			MountInfo *minfo = &m_Mount[abspath];
			minfo->mountpath = abspath;
			minfo->handle = 0;
			GetFileInfo(abspath, minfo->mountinfo);

			// scan total archive files
			// (however, don't read it yet - it costs much.)
			struct archive_entry *aent;
			ITER_ARCHIVE(a, aent)
			{
				if (r == ARCHIVE_OK) {
					const char* fn = archive_entry_pathname(aent);
					if (!fn) continue;	// sometimes null, why ...??

					// cache archived file info
					FileInfo finfo;
					finfo.updatetime = archive_entry_atime(aent);
					finfo.createtime = archive_entry_birthtime(aent);
					finfo.type = archive_entry_filetype(aent) == AE_IFREG ? FILETYPE::TYPE_FILE : FILETYPE::TYPE_FOLDER;
					finfo.size = archive_entry_size(aent);
					finfo.data = 0;
					finfo.filename = fn;
					minfo->files.push_back(finfo);

					// skip read data(do it LoadFile() as it costs a lot)
					// just go to next entry.
					archive_read_data_skip(a);
				}
				else if (r == ARCHIVE_FATAL) {
					break;
				}
			}
			//archive_entry_free(aent);
			CloseArchive(a);
		}
		// failed to open archive file, return false
		else
			return false;
	}
	else return FileManagerBasic::Mount(path);
}

void FileManager::UnMount(const RString& path) {
	// we do some additional process
	// like releasing archive/file pointer
	auto iter = m_Mount.find(path);
	if (iter != m_Mount.end()) {
		MountInfo* minfo = &iter->second;
		if (minfo->handle) archive_read_close((archive*)minfo->handle);
		for (auto it2 = minfo->files.begin(); it2 != minfo->files.end(); ++it2) {
			if (it2->data) free(it2->data);
		}
		FileManagerBasic::UnMount(path);
	}
}

FileBasic* FileManager::LoadFile(const RString& path) {
	// if dirpath is archive,
	// then check is archive raw data is loaded
	RString dirpath = GetDirectory(path);
	if (FileHelper::IsArchiveFileName(dirpath)) {
		auto iter = m_Mount.find(dirpath);
		if (iter != m_Mount.end()) {
			// pick any file
			// if it's empty, then load it
			if (iter->second.files.size() && iter->second.files[0].data == 0) {
				archive *a = archive_read_new();
				if (OpenArchive(a, dirpath) == 0) {
					// CAUTION:
					// This just caches file without checking filename.
					// It's a little unsafe if archive iterates file randomly
					// but that's no possibility of that, so keep going on.
					// (in case of something going wrong, just put a ASSERT).
					struct archive_entry *aent;
					int fi = 0;
					ITER_ARCHIVE(a, aent)
					{
						if (r == ARCHIVE_OK) {
							FileInfo* finfo = &iter->second.files[fi];
							ASSERT(stricmp(finfo->filename, archive_entry_pathname(aent)) == 0);
							// start to read file
							const void *ptr_;
							int64_t offset_;
							size_t size_;
							void* data_ = malloc(finfo->size);
							while ((r = archive_read_data_block(a, &ptr_, &size_, &offset_)) == 0) {
								memcpy((char*)data_ + offset_, ptr_, size_);
							}
							if (r == ARCHIVE_FAILED) {
								LOG->Warn("Archive file(%s) seems like damaged (unzipping error)...", dirpath.c_str());
								// We'd better to delete dummy `data_`,
								// but there's no way to distinguish unattempted file from attempted one.
								// So, just add dummy ptr to data (It'll automatically released, naturally.)
								LOG->Warn(archive_error_string(a));
							}
							finfo->data = data_;
						}
						else {
							LOG->Warn("Archive file(%s) seems like damaged (parse error)...", dirpath.c_str());
							if (r == ARCHIVE_FATAL) {
								// stop reading archive file
								break;
							}
						}
						fi++;
					}
					// raw data filling end.
					CloseArchive(a);
				}
			}
		}
	}

	// now call basic handler
	return FileManagerBasic::LoadFile(path);
}

bool FileManager::ReadAllFile(const RString& path, char **p, int *len) {
	// just rewrite func to replace FileManagerBasic::LoadFile().
	FileBasic* f = LoadFile(path);
	if (!f)
		return false;

	*len = f->GetSize();
	f->Reset();
	*p = (char*)malloc(*len);
	f->ReadAll(*p);
	f->Close();
	return true;
}








/*
 * helper for file class
 */

namespace FileHelper {
	RString ReplacePathEnv(const RString& path) {
		int p = 0, p2 = 0;
		RString rpath = path;
		while (p != RString::npos) {
			p = rpath.find("$(", p);
			if (p == RString::npos)
				break;
			p2 = rpath.find(")", p);
			if (p2 != RString::npos) {
				RString key = rpath.substr(p + 2, p2 - p - 2);
				RString val = "";
				if (STRPOOL->IsExists(key))
					val = *STRPOOL->Get(key);
				rpath = rpath.substr(0, p) + val + rpath.substr(p2 + 1);
				p = p2 + 1;
				if (p >= rpath.size())
					break;
			}
			else {
				break;
			}
		}
		return rpath;
	}

	bool GetAnyAvailableFilePath(RString &path) {
		// first attempt to find file existence ..
		path = FILEMANAGER->GetAbsolutePath(path);
		if (FILEMANAGER->IsFile(path) > FILETYPE::TYPE_NULL)
			return true;

		// if not, then find any file with same extension
		// if (substitute_extension(get_filename(_path), "") == "*")
		int p = path.find_last_of('.');
		RString ext = "";
		if (p != std::string::npos) {
			ext = path.substr(p);
		}
		FILEMANAGER->SetFileFilter(ext);
		RString folder = FILEMANAGER->GetDirectory(path);

		std::vector<RString> filelist;
		FILEMANAGER->GetDirectoryFileList(folder, filelist);
		if (filelist.size() > 0) {
			// randomly select any file
			path = folder + filelist[rand() % filelist.size()];
			return true;
		}

		// nothing found
		return false;
	}

	bool IsFile(const RString& path) {
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(FILEMANAGER->GetAbsolutePath(path));
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

	bool IsDirectory(const RString& path) {
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(FILEMANAGER->GetAbsolutePath(path));
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

	bool GetFileInfo(const RString& path, FileInfo& finfo) {
#ifdef _WIN32
		std::wstring path_w = RStringToWstring(path);
		struct _stat64i32 s;
		if (_wstat(path_w.c_str(), &s) != 0) return false;
		finfo.type = (s.st_mode & S_IFDIR) ? FILETYPE::TYPE_FOLDER : FILETYPE::TYPE_FILE;
		finfo.data = 0;
		finfo.createtime = s.st_ctime;
		finfo.updatetime = s.st_mtime;
		finfo.size = s.st_size;
		finfo.filename = path;
		return true;
#else
		struct stat s;
		if (stat(path.c_str(), &s) != 0) return false;
		finfo.type = (s.st_mode & S_IFDIR) ? FILETYPE::TYPE_FOLDER : FILETYPE::TYPE_FILE;
		finfo.data = 0;
		finfo.createtime = st->st_ctime;
		finfo.updatetime = st->st_mtime;
		finfo.size = st->st_size;
		finfo.filename = path;
		return true;
#endif
	}

	// also checks for archive format file path
	bool IsArchiveFileName(const RString& path) {
		RString tmp = path;
		if (tmp.back() == '/' || tmp.back() == '\\') tmp.pop_back();
		return EndsWith(tmp, ".zip") ||
			EndsWith(tmp, ".rar") ||
			EndsWith(tmp, ".7z");
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
	bool CreateDirectory(const RString& path) {
		// if current directory is not exist
		// then get parent directory
		// and check it recursively
		// after that, create own.
		if (IsDirectory(path))
			return true;
		if (IsFile(path))
			return false;
		if (!_create_directory(path)) {
			RString parent = FileManager::GetParentDirectory(path);
			if (NOT(CreateDirectory(parent))) {
				return false;
			}
			return _create_directory(path);
		}
		else return true;
	}
}




FileManager *FILEMANAGER = new FileManager();