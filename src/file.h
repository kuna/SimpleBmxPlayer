/*
 * @description
 * Manages File I/O for game.
 * Since BMS Files and resources has various structure (archive or directory structure),
 * We need to make this class to make ease of it.
 *
 * Goal: Give File Handler to -
 * - Raw Data
 * - Zip archive (kind of directory)
 * - Directory
 * - General file
 * General operation for file: Seek, Read, GetLine, ...
 * General operation for directory: GetAllFiles, GetExt, GetAllDirectories, ...
 */

#pragma once

#include "SDL/SDL.h"
#include "global.h"
#include <vector>
#include <map>

class FileBasic {
public:
	virtual int ReadLine(RString &str) = 0;
	virtual int ReadAll(RString &str) = 0;
	virtual int ReadAll(char *p) = 0;
	virtual int Read(RString &str, size_t size) = 0;
	virtual int Read(char *p, size_t size) = 0;
	virtual int Write(const RString &in) = 0;
	virtual int Write(const char* in, int size) = 0;
	virtual int WriteToFile(const char* path) = 0;
	virtual void Close() = 0;
	virtual size_t GetSize() = 0;
	virtual bool IsEOF() = 0;
	virtual void Reset() = 0;
	virtual void Seek(size_t pos) = 0;
	virtual SDL_RWops* GetSDLRW() = 0;

	/* not implemented but necessary methods */
	bool GetCRC32(uint32_t *iRet);
	bool GetBase64(RString* out);
	bool SetBase64(RString* in);
	RString GetMD5Hash();
};

class File: public FileBasic {
private:
	FILE *fp;
#if _WIN32
	std::wstring fpath_w;
	std::wstring fmode_w;
#endif
	std::string fpath;
	std::string fmode;
public:
	File();
	File(const char* filename, const char* mode = "r");
	~File();
	bool Open(const char* filename, const char* mode = "r");
	bool IsOpened() { return fp != 0; }
	virtual void Close();

	virtual int ReadLine(RString &str);
	virtual int ReadAll(RString &str);
	virtual int ReadAll(char *p);
	virtual int Read(RString &str, size_t size);
	virtual int Read(char *p, size_t size);
	virtual int Write(const RString &in);
	virtual int Write(const char* in, int size);
	virtual int WriteToFile(const char* path);
	virtual size_t GetSize();
	virtual bool IsEOF();
	virtual void Reset();
	virtual void Seek(size_t pos);
	virtual SDL_RWops* GetSDLRW();

	RString& GetFilePath();
};

class FileMemory : public FileBasic {
public:
	/*
	 * returns memory object used for SDL
	 * (dont modify file data during SDL works with this handle)
	 */
	FileMemory();
	FileMemory(const char* data, size_t size);
	void Set(const char* data, size_t size);
	virtual int ReadLine(RString &str);
	virtual int ReadAll(RString &str);
	virtual int ReadAll(char *p);
	virtual int Read(RString &str, size_t size);
	virtual int Read(char *p, size_t size);
	virtual int Write(const RString &in);
	virtual int Write(const char* in, int size);
	virtual int WriteToFile(const char* path);
	virtual void Close();
	virtual bool IsEOF();
	virtual void Reset();
	virtual void Seek(size_t pos);
	virtual size_t GetSize();
	virtual SDL_RWops* GetSDLRW();
private:
	std::vector<char> m_data;
	size_t m_pos;
};

enum FILETYPE {
	TYPE_MOUNT = 3,
	TYPE_FOLDER = 2,
	TYPE_FILE = 1,
	TYPE_NULL = 0,
};

struct FileInfo {
	RString filename;
	FILETYPE type;
	time_t createtime;
	time_t updatetime;
	size_t size;
	void* data;
};

/*
 * @description
 * This class only accesses to OS basic I/O, not archive.
 */
class FileManagerBasic {
protected:
	std::vector<RString> m_Basepath;
	std::vector<RString> m_Filterext;
	bool m_SearchMountPath;

	typedef struct {
		RString mountpath;		// only absolute directory path!
		FileInfo mountinfo;
		std::vector<FileInfo> files;
		void* handle;
	} MountInfo;
	std::map<RString, MountInfo> m_Mount;
public:
	~FileManagerBasic();

	/* Set base path or relative path (must be called at once) */
	virtual void PushBasePath(const RString& path);
	virtual void PopBasePath();
	RString GetBasePath();
	void SetFileFilter(const RString& filter);
	void SearchMountFolder(bool v) { m_SearchMountPath = v; }

	/* info */
	virtual bool GetFileInfo(const RString& path, FileInfo &info);	// also available for directory
	virtual void GetDirectoryFileList(const RString& dirpath, std::vector<RString> files);	// this function isn't thread safe - caution
	virtual void GetDirectoryFileInfo(const RString& dirpath, std::vector<FileInfo> files);	// this function isn't thread safe - caution
	bool IsFile(const RString& path);
	bool IsDirectory(const RString& path);
	bool IsMountedFile(const RString& path);

	/* these functions doesn't gurantee file existing */
	static bool IsAbsolutePath(const RString& path);
	RString GetAbsolutePath(const RString& path);
	bool GetExistingAbsolutePath(const RString& path, RString& out);
	RString GetRelativePath(const RString& path);	// returns relative path to basepath
	RString GetRelativePath(const RString& path, const RString& basepath);
	static RString GetDirectory(const RString& path);
	static RString GetParentDirectory(const RString& path);

	/* mounting */
	virtual bool Mount(const RString& path);
	virtual void UnMount(const RString& path);
	
	/* I/O */
	virtual FileBasic* LoadFile(const RString& path);
	virtual bool ReadAllFile(const RString& path, char **p, int *len);
	virtual bool CreateFile(const RString& path);
	virtual bool CreateDirectory(const RString& path);
};

/*
 * @description
 * This class is available to mount archive file.
 */
class FileManager: public FileManagerBasic {
public:
	FileManager();
	~FileManager();

	virtual void PushBasePath(const RString& path);

	virtual bool Mount(const RString& path);
	virtual void UnMount(const RString& path);

	virtual FileBasic* LoadFile(const RString& path);
	virtual bool ReadAllFile(const RString& path, char **p, int *len);
};

// global accessible
extern FileManager* FILEMANAGER;


namespace FileHelper {
	/*
	 * Mostly used by Skin
	 */
	RString ReplacePathEnv(const RString& path);
	bool GetAnyAvailableFilePath(RString &path);

	bool IsArchiveFileName(const RString& path);
	/*
	 * low-level
	 */
	bool GetFileInfo(const RString& path, FileInfo &info);
	bool IsFile(const RString& path);
	bool IsDirectory(const RString& path);
	bool CreateDirectory(const RString& dirpath);
}