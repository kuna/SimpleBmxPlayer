#include "audio.h"
#include "util.h"

#ifdef _WIN32
Audio::Audio(std::wstring& filepath, int channel) : channel(channel), sdlaudio(0) {
	RString path_utf8 = WStringToRString(filepath);
	Load(path_utf8, channel);
}
#endif

Audio::Audio() : channel(-1), sdlaudio(0) {}

Audio::Audio(const char* filepath, int channel) : channel(channel), sdlaudio(0) {
	Load(filepath, channel);
}

Audio::~Audio() {
	Close();
}

bool Audio::Load(const char* filepath, int channel = -1) {
	Close();
	File *f = new File(filepath, "rb");
	bool r = Load(f, channel);
	delete f;
	return r;
}

bool Audio::Load(FileBasic* f, int channel = -1) {
	Close();
	sdlaudio = Mix_LoadWAV_RW(f->GetSDLRW(), 1);
	this->channel = channel;
	if (sdlaudio)
		return true;
	else
		return false;
}

void Audio::Close() {
	if (IsLoaded())
		Mix_FreeChunk(sdlaudio);
}

bool Audio::IsLoaded() {
	return sdlaudio != 0;
}
void Audio::Play() {
	Mix_PlayChannel(channel, sdlaudio, 0);
}

void Audio::Stop() {
	Mix_HaltChannel(channel);
}

Uint32 Audio::GetLength() {
	if (!IsLoaded()) 
		return 0;
	else 
		return sdlaudio->alen / 4 * 1000 / 44100;
}

// http://stackoverflow.com/questions/19200033/match-duration-using-sdl-mixer
void Audio::Resample(double rate) {
	if (!IsLoaded()) return;
	if (rate == 1) return;

	int m = 1, n = 1;
	Mix_Chunk *newsample = (Mix_Chunk*)SDL_malloc(sizeof(Mix_Chunk));
	int original_length = sdlaudio->alen;
	int length = original_length * rate;
	
	int allocsize = sizeof(Uint8) * length;
	newsample->allocated = allocsize;
	newsample->abuf = NULL;
	newsample->alen = length;
	newsample->volume = sdlaudio->volume;
	Uint8 *data1 = (Uint8*)SDL_malloc(allocsize);// new Uint8[length];
	Uint8 *start = sdlaudio->abuf;		// position of old buffer
	Uint8 *start1 = data1;				// position of new buffer

	int size = 16;

	if (size == 32) {
		Uint32 sam;

		for (; m <= original_length; m += 4) {
			sam = *((Uint32 *)start);
			for (; n <= rate * m; n += 4) {
				*((Uint32 *)start1) = sam;
				start1 += 4;
			}
			start += 4;
		}
	}
	else if (size == 16) {
		Uint16 sam;
		for (; m <= original_length; m += 2) {
			sam = *((Uint16 *)start);
			for (; n <= rate * m; n += 2) {
				*((Uint16 *)start1) = sam;
				start1 += 2;
			}
			start += 2;
		}

	}
	else if (size == 8) {
		Uint8 sam;
		for (; m <= original_length; m ++) {
			sam = *start;
			for (; n <= rate * m; n++) {
				*start1 = sam;
				start1++;
			}
			start++;
		}


	}
	newsample->abuf = data1;
	Mix_FreeChunk(sdlaudio);
	sdlaudio = newsample;
}