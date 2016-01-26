#pragma once

#include "global.h"
#include "SDL/SDL_mixer.h"

class Audio {
private:
	Mix_Chunk *sdlaudio;
	int channel;
public:
	Audio(const char* filepath, int channel = -1);
	Audio(std::wstring& filepath, int channel=-1);
	~Audio();
	bool IsLoaded();
	void Play();
	void Stop();
	Uint32 GetLength();
	void Resample(double rate);
};
