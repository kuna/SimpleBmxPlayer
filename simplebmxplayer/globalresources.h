/*
 * @description
 * Stores & Manages Image/Audio/String/Number ... objects used in game. (not BMS)
 * global accessible.
 */

#pragma once

#include <map>
#include <vector>
#include "image.h"
#include "audio.h"
#include "timer.h"
#include "global.h"
#include "font.h"

using namespace std;

class StringPool {
private:
	std::map<RString, RString> _stringpool;
public:
	bool IsExists(const RString &key);
	RString* Set(const RString &key, const RString &value = "");
	RString* Get(const RString &key);
	void Clear();
};

class DoublePool {
private:
	std::map<RString, double> _doublepool;
public:
	bool IsExists(const RString &key);
	double* Set(const RString &key, double value = 0);
	double* Get(const RString &key);
	void Clear();
};

class IntPool {
private:
	std::map<RString, int> _intpool;
public:
	bool IsExists(const RString &key);
	int* Set(const RString &key, int value = 0);
	int* Get(const RString &key);
	void Clear();
};

class TimerPool {
private:
	std::map<RString, Timer> _timerpool;
public:
	bool IsExists(const RString &key);
	/** @brief just set timer if not exists. different from Get (this always creates and tick if not exists) */
	Timer* Set(const RString &key, bool activate = true);
	void Reset(const RString &key);
	void Stop(const RString &key);
	/** @brief return 0 if timer not exists. */
	Timer* Get(const RString &key);
	/** @brief refer to Timer::Trigger(bool) */
	bool Trigger(const RString &key, bool condition);
	void Clear();
};

typedef void (*_Handler)(void*);
class HandlerPool {
private:
	std::map<RString, std::vector<_Handler>> _handlerpool;
public:
	bool IsExists(const RString &key);
	_Handler Add(const RString &key, _Handler h);
	bool Call(const RString &key, void* arg);
	bool Remove(const RString &key, _Handler h);
	void Clear();
};

class ImagePool {
private:
	std::map<RString, Image*> _imagepool;
	std::map<Image*, int> _loadcount;
public:
	~ImagePool();
	void ReleaseAll();

	bool IsExists(const RString &path);
	Image* Load(const RString &path);
	bool Release(Image *img);
	Image* Get(const RString &path);
};

// TODO: namepool / pathpool is different. we should make them different.
// don't make it confusing ...
class FontPool {
private:
	std::map<RString, Font*> _fontpool;
	std::map<Font*, int> _loadcount;
public:
	~FontPool();
	void ReleaseAll();

	bool IsExists(const RString &id);
	bool IsIDExists(const RString &id);
	Font* LoadTTFFont(const RString &path, int size, SDL_Color color, int thickness = 0,
		SDL_Color bordercolor = FC_MakeColor(0, 0, 0, 255), int border = 1,
		int style = TTF_STYLE_NORMAL, const char* texturepath = 0);
	Font* LoadTTFFont(const RString &id, const RString &path, int size, SDL_Color color, int thickness = 0,
		SDL_Color bordercolor = FC_MakeColor(0, 0, 0, 255), int border = 1,
		int style = TTF_STYLE_NORMAL, const char* texturepath = 0);
	Font* LoadTextureFont(const RString &path);
	Font* LoadTextureFontFromTextData(const RString &id, const RString &textdata);
	Font* Get(const RString &path);
	Font* GetByID(const RString &id);
	bool Release(Font *f);
};

class SoundPool {
private:
	std::map<RString, Audio*> _soundpool;
	std::map<Audio*, int> _loadcount;
public:
	~SoundPool();
	void ReleaseAll();

	bool IsExists(const RString &key);
	Audio* Load(const RString &path);
	bool Release(Audio* audio);
	Audio* Get(const RString &path);
};

extern StringPool* STRPOOL;
extern DoublePool* DOUBLEPOOL;
extern IntPool* INTPOOL;
extern TimerPool* TIMERPOOL;
extern HandlerPool* HANDLERPOOL;

extern ImagePool* IMAGEPOOL;
extern FontPool* FONTPOOL;
extern SoundPool* SOUNDPOOL;

namespace PoolHelper {
	void InitalizeAll();
	void ReleaseAll();
}

#define SWITCH_ON(s) (TIMERPOOL->Set(s, true))
#define SWITCH_OFF(s) (TIMERPOOL->Set(s, false))