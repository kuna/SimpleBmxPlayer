#pragma once

#include "global.h"
#include "file.h"
#include "SDL/SDL_mixer.h"

class Audio {
private:
	Mix_Chunk *sdlaudio;
	int channel;
public:
#ifdef _WIN32
	Audio(std::wstring& filepath, int channel=-1);
#endif
	Audio();
	Audio(const char* filepath, int channel = -1);
	~Audio();
	bool Load(const char* filepath, int channel = -1);
	bool Load(FileBasic* f, int channel = -1);
	void Close();
	bool IsLoaded();
	void Play();
	void Stop();
	Uint32 GetLength();
	void Resample(double rate);
};

// COMMENT: change load function to high-level one (using file method, to make available from archive.)