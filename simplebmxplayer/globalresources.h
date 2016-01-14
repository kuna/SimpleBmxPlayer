/*
 * @description
 * Stores & Manages Image/Audio/String/Number ... objects used in game. (not BMS)
 * global accessible.
 */

#pragma once

#include <map>
#include "image.h"
#include "audio.h"
#include "timer.h"
#include "global.h"

using namespace std;

class StringPool {
private:
	std::map<RString, RString> _stringpool;
public:
	bool IsExists();
	void Set(RString &key, const RString &value = "");
	RString& Get(RString &key);
	void Clear();
};

class DoublePool {
private:
	std::map<RString, double> _doublepool;
public:
	bool IsExists();
	void Set(RString &key, double value = 0);
	double* Get(RString &key);
	void Clear();
};

class IntPool {
private:
	std::map<RString, int> _intpool;
public:
	bool IsExists();
	void Set(RString &key, int value = 0);
	int* Get(RString &key);
	void Clear();
};

class TimerPool {
private:
	std::map<RString, Timer> _timerpool;
public:
	bool IsExists();
	void Set(RString &key, bool activate = true);
	void Reset(RString &key);
	void Stop(RString &key);
	Timer* Get(RString &key);
	/** @brief if (condition && !timer), then return true and start timer, otherwise false. */
	bool Trigger(RString &key, bool condition);
	void Clear();
};

typedef void (*_Handler)(void*);
class HandlerPool {
private:
	std::map<RString, _Handler> _handlerpool;
public:
	bool IsExists();
	void Add(RString &key, _Handler h);
	_Handler Get(RString &key);
	bool Call(RString &key, void* arg);
	void Remove(RString &key);
	void Clear();
};

class ImagePool {
private:
	std::map<RString, Image*> _imagepool;
	std::map<Image*, int> _loadcount;
public:
	~ImagePool();
	void ReleaseAll();

	Image* Load(RString &path);
	void Release(Image *img);
	Image* Get(RString &path);
};

/* NOT IMPLEMENTED */
class FontPool {
private:
public:
};

class SoundPool {
private:
	std::map<RString, Audio*> _soundpool;
	std::map<Audio*, int> _loadcount;
public:
	~SoundPool();
	void ReleaseAll();

	Audio* Load(RString &path);
	void Release(Audio* audio);
	Audio* Get(RString &path);
};

extern StringPool* STRPOOL;
extern DoublePool* DOUBLEPOOL;
extern IntPool* INTPOOL;
extern HandlerPool* HANDLERPOOL;

extern ImagePool* IMAGEPOOL;
extern FontPool* FONTPOOL;
extern SoundPool* SOUNDPOOL;