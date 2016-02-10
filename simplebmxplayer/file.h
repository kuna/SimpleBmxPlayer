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

class FileBasic {
public:
	virtual int ReadLine(RString &str) = 0;
	virtual int ReadAll(RString &str) = 0;
	virtual int ReadAll(char *p) = 0;
	virtual int Read(RString &str, size_t size) = 0;
	virtual int Read(char *p, size_t size) = 0;
	virtual int Write(const RString &in) = 0;
	virtual int Write(const char* in, int size) = 0;
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

namespace FileHelper {
	/*
	 * @brief
	 * set basepath. useful for ConvertPathToAbsolute()
	 * TODO: if archive file given as input, then open archive file(mount).
	 */
	void PushBasePath(const char *path);
	void PopBasePath();
	RString& GetBasePath();
	RString& GetSystemPath();
	void GetFileList(const char *folderpath, std::vector<RString>& filelist, bool getfileonly = true);
	void GetFileList(std::vector<RString>& filelist);
	void FilterFileList(const char *extfilters, std::vector<RString>& filelist);
	/** @brief is path exists & file? */
	bool IsFile(const RString& path);
	/** @brief is path exists & folder? */
	bool IsFolder(const RString& path);

	bool IsRelativeFile(const RString& relpath);
	/** @brief create folder (recursively), return false if failed. */
	bool CreateFolder(const RString& path);
	/** @brief just get parent directory path. no IO function. */
	RString GetParentDirectory(const RString& path);
	/** @brief replace some env into valid string (refers STRINGPOOL) */
	void ReplacePathEnv(RString& path);
	/** @brief converts path to absolute path */
	void ConvertPathToAbsolute(RString& path);
	void ConvertPathToSystem(RString& path);
	/* @description
	 * this method tries these paths:
	 * 1. path itself
	 * 2. converted absolute path
	 * 3. any(random) file in that directory (with same extension)
	 * if all of these are failed, return false.
	 */
	bool GetAnyAvailableFilePath(RString& path);

	bool LoadFile(const char *relpath, FileBasic **f);
	bool CurrentDirectoryIsZipFile();
	bool GetDirDate(const char* relpath);
}