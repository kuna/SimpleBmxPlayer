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

#include "global.h"

class FileBasic {
public:
	virtual int ReadLine(RString &str) = 0;
	virtual int ReadAll(RString &str) = 0;
	virtual int ReadAll(char *p) = 0;
	virtual int Read(RString &str, size_t size) = 0;
	virtual size_t GetFileSize() = 0;
	virtual bool IsEOF() = 0;
	virtual void Reset() = 0;
	virtual void Seek(size_t pos) = 0;

	/* not implemented but necessary methods */
	bool GetCRC32(uint32_t *iRet);
	RString GetMD5Hash();
};

class File: public FileBasic {
private:
	FILE *fp;
	RString fname;
public:
	File();
	~File();
	bool Open(const char* filename, const char* mode = "r");
	void Close();

	virtual int ReadLine(RString &str);
	virtual int ReadAll(RString &str);
	virtual int ReadAll(char *p);
	virtual int Read(RString &str, size_t size);
	virtual size_t GetFileSize();
	virtual bool IsEOF();
	virtual void Reset();
	virtual void Seek(size_t pos);

	RString& GetFilePath();
};

namespace FileHelper {
	/** @brief set basepath. useful for ConvertPathToAbsolute() */
	void PushBasePath(const char *path);
	void PopBasePath();
	RString& GetBasePath();
	/** @brief converts path to absolute path */
	void ConvertPathToAbsolute(RString& path);
}