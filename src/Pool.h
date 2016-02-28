/*
 * @description
 * Stores & Manages Image/Audio/String/Number ... objects used in game. (not BMS)
 * global accessible.
 */

#pragma once

#include <map>
#include <vector>
#include "Surface.h"
#include "audio.h"
#include "timer.h"
#include "global.h"
#include "font.h"

using namespace std;






// basic util

struct Message {
	RString name;
};

class Handler {
public:
	virtual void Receive(const Message& msg) = 0;
};

class HandlerAuto : public Handler {
public:
	HandlerAuto();
	~HandlerAuto();
	virtual void Receive(const Message& msg) = 0;
};






// pools

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

/* DEPRECIATED */
#if 0
class TimerPool {
private:
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
#endif

/*
 * When handler called, not only all registered classes are called
 * but also inner timer starts.
 * - If timer off, then only timer goes off (don't effect to timerpool)
 * - If timer(switch) already on, then calling Trigger() method won't take effect.
 *   Instead, use Reset() instead.
 */
class HandlerPool {
private:
	std::vector<Handler*> _handlerpool;
	std::map<RString, Timer> _timerpool;
public:
	// basic element operation
	void Clear();
	void Register(Handler* handler);
	void Remove(Handler* h);
	bool IsExists(const RString &key);
	bool IsRegistered(Handler* h);
	bool IsStarted(const RString& name);
	Timer* Get(const RString &name);

	// handler related
	bool Trigger(const RString &name, bool condition = true);
	Timer* Reset(const RString &name);
	Timer* Stop(const RString &name);
};

class TexturePool {
private:
	std::map<RString, Display::Texture*> _texpool;
	std::map<Display::Texture*, Surface*> _surface;
	std::map<Display::Texture*, int> _loadcount;
public:
	~TexturePool();
	void ReleaseAll();

	bool IsExists(const RString &path);
	Display::Texture* Load(const RString &path);
	Display::Texture* Register(const RString& key, Display::Texture *tex);
	bool Release(Display::Texture *img);
	Display::Texture* Get(const RString &path);		// this doesn't make load count up
	void Update(Display::Texture* tex, Uint32 msec);	// update texture surface, in case of movie
};

// FontPool is a little different from imagepool 
// It just stores fonts but don't emits it.
// so, you should Release it by yourself (to prevent memory leak)
// but you may can make ID and get Font* from that.
// (ID randomly created if you don't set it)
class FontPool {
private:
	std::map<RString, Font*> _fontpool;
public:
	~FontPool();
	void ReleaseAll();
	bool Release(Font *f);
	bool IsIDExists(const RString &id);
	Font* GetByID(const RString &id);

	Font* RegisterFont(const char* id, Font* f);
	Font* LoadTTFFont(const char* id, const RString &path, int size, Uint32 color, int thickness = 0,
		Uint32 bordercolor = 0x000000FF, int border = 1,
		int style = 0, const char* texturepath = 0);
	Font* LoadTextureFont(const char* id, const RString &path);
	Font* LoadTextureFontFromTextData(const char* id, const RString &textdata);
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
extern HandlerPool* HANDLERPOOL;

extern TexturePool* TEXPOOL;
extern FontPool* FONTPOOL;
extern SoundPool* SOUNDPOOL;

namespace PoolHelper {
	void InitalizeAll();
	void ReleaseAll();
}

/*
 * I suggest to use macro in switch -
 * as internal structure can be changed in any case.
 */
#define SWITCH_ON(s) (HANDLERPOOL->Reset(s))
#define SWITCH_OFF(s) (HANDLERPOOL->Stop(s))
#define SWITCH_TRIGGER(s, cond) (HANDERPOOL->Trigger(s, cond))
#define SWITCH_GET(s) (HANDLERPOOL->Get(s))