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




template <class T>
class BasicPool {
protected:
	std::map<RString, T> m_Pool;
public:
	BasicPool<T>() {  };
	~BasicPool<T>() { ReleaseAll(); };
	void ReleaseAll() { m_Pool.clear(); }

	bool IsExists(const RString &key) { return (m_Pool.find(key) != m_Pool.end()); };
	T* Set(const RString &key, T v) { m_Pool[key] = v; };
	T* Get(const RString &key) { auto it = m_Pool.find(key); if (it == m_Pool.end()) return 0; else return *it; };
	void Remove(const RString& key) { m_Pool.erase(key); };
	void Clear() { m_Pool.clear(); };
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
 * Switch is an extended class from timer:
 */
class Switch: public Timer {
protected:
	RString m_Switchname;
public:
	/* Although non-type switch is bad choice, but std::map requies default constructor, 
	   so There's only way to filter it with assert() */
	Switch(const RString& name = "", int state = TIMERSTATUS::UNKNOWN);
	virtual void Start();
	virtual void Stop();
	virtual bool Trigger(bool condition = true);
};

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
	std::map<RString, Switch> _timerpool;

	friend class Switch;
	void CallHandler(const RString &name);
public:
	// basic element operation
	void Clear();
	void Register(Handler* handler);
	void RemoveHandler(Handler* h);
	void Remove(const RString& key);
	bool IsExists(const RString &key);
	bool IsRegistered(Handler* h);
	bool IsStarted(const RString& name);

	// handler related
	Switch* Start(const RString &name);
	Switch* Stop(const RString &name);

	Switch* Get(const RString &name);
};

class TexturePool {
private:
	std::map<RString, Display::Texture*> _texpool;
	std::map<Display::Texture*, int> _loadcount;
public:
	~TexturePool();
	void ReleaseAll();

	bool IsExists(const RString &path);
	Display::Texture* Load(const RString &path);
	Display::Texture* Register(const RString& key, Display::Texture *tex);
	bool Release(Display::Texture *img);
	Display::Texture* Get(const RString &path);		// this doesn't make load count up
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

extern BasicPool<RString>* STRPOOL;
extern BasicPool<double>* DOUBLEPOOL;
extern BasicPool<int>* INTPOOL;
extern HandlerPool* HANDLERPOOL;

extern TexturePool* TEXPOOL;
extern FontPool* FONTPOOL;
extern SoundPool* SOUNDPOOL;



template <class T>
class Value {
protected:
	T *m_Ptr = 0;
public:
	T* SetPtr(T* p) { m_Ptr = p; return p; }
	T* GetPtr() { return m_Ptr; }
	Value<T>& Set(T v) { *m_Ptr = v; return this; }
	T Get() const { if (m_Ptr) return *m_Ptr; else return T(); }
	T operator=(T v) { return (*m_Ptr = v); }
	T operator=(T *v) { return (m_Ptr = v); }
	operator const T() const { return Get(); }

	Value<T>& SetFromPool(const RString& name);
	Value<T>& SetFromPool(int player, const RString& name);		// only for player
};

class SwitchValue: public Value<Switch> {
public:
	void Start();
	void Stop();
	void Pause();
	bool Trigger(bool condition = true);
	//bool OffTrigger(bool condition = true);

	uint32_t GetTick();
	bool IsStarted();
};





/*
 * I suggest to use macro in switch -
 * as internal structure can be changed in any case.
 */
#define SWITCH_ON(s) (HANDLERPOOL->Start(s))
#define SWITCH_OFF(s) (HANDLERPOOL->Stop(s))
#define SWITCH_TRIGGER(s, cond) (HANDERPOOL->Trigger(s, cond))
#define SWITCH_GET(s) (HANDLERPOOL->Get(s))



namespace PoolHelper {
	void InitializeAll();
	void ReleaseAll();
}