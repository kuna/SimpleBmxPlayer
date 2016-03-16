#pragma once

#define _USEPOOL				// to use SetEnvironmentFromOption()
#include "Skin.h"
#include "Display.h"			// DISPLAY dependent
#include "Font.h"				// FONT dependent
#include "Audio.h"
#include "Timer.h"

class Actor;
typedef Actor* (*CreateActorFn)();


/*
 * @description
 * manages to load & render skin data
 *
 * This class SHOULD NOT run in multiple time,
 * because that may cause collsion with lua variable.
 * (Although lua will be automatically locked ...)
 */
class Theme {
protected:
	// skin related information
	Skin m_Skin;
	SkinMetric m_Skinmetric;
	SkinOption m_Skinoption;

	// rendering object
	Actor* base_;
public:
	Actor* GetPtr();
	void Update();
	void Render();
	bool Load(const char* skinname);
	void ClearElements();

	// create actor from element
	static Actor* MakeActor(const tinyxml2::XMLElement* e, Actor* parent = 0);
	// register actor generate func
	static void RegisterActor(const std::string& name, CreateActorFn fn);
	// used when pool's resource are being cleared
	static void ClearAllResource();

	static Display::Texture* GetTexture(const std::string id);
	static Font* GetFont(const std::string id);
};


// ----------------------- Pool -------------------------

template <class T>
class BasicPool {
protected:
	std::map<RString, T> m_Pool;
public:
	BasicPool<T>() {  };
	~BasicPool<T>() { ReleaseAll(); };
	void ReleaseAll() { m_Pool.clear(); }

	bool IsExists(const RString &key) { return (m_Pool.find(key) != m_Pool.end()); };
	T* Set(const RString &key, T v) { m_Pool[key] = v; return &m_Pool[key]; };
	T* Get(const RString &key) { auto it = m_Pool.find(key); if (it == m_Pool.end()) return 0; else return &it->second; };
	void Remove(const RString& key) { m_Pool.erase(key); };
	void Clear() { m_Pool.clear(); };
};

class Event {
private:
	RString eventname;
	std::vector<Handler*> handlers;
	Timer timer;

public:
	Event(const RString& name);
	void Start();					// always trigger, regardless of current status
	void Call() { Start(); };		// alias for Start()
	void Stop();
	bool Trigger(bool c = true);	// only start when timer isn't started
	void Pause();
	bool IsStarted();

	void Register(Handler* h);
	void UnRegister(Handler *h);
	Timer* GetTimer() { return &timer; };
	typedef std::vector<Handler*>::iterator Iterator;
	Iterator Handler_Begin() { return handlers.begin(); }
	Iterator Handler_End() { return handlers.end(); }

	uint32_t GetTick();

	void SetArgument(int idx, const RString& arg);
	RString GetArgument(int idx);
};

/*
 * WARN
 * Propagating Event may be thread-unsafe (because of Argument),
 * So may be better to make global mutex in Event::Start().
 * */
class EventPool {
private:
	std::map<RString, Event> _eventpool;
	RString argv[10];
	//int argc;
public:
	// basic element operation
	void Clear();

	// handler related
	void Register(const RString& key, Handler* h);
	void UnRegister(const RString& key, Handler* h);
	void UnRegisterAll(Handler* h);
	bool IsExists(const RString &key);
	void Remove(const RString& key);
	Event* Get(const RString &name);

	// argument
	void SetArgument(int idx, const RString& arg);
	RString GetArgument(int idx);
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
extern EventPool* EVENTPOOL;

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
	T* operator=(T *v) { return (m_Ptr = v); }
	operator const T() const { return Get(); }

	Value<T>& SetFromPool(const RString& name);
	Value<T>& SetFromPool(int player, const RString& name);		// only for player
};

// just for convenience
class EventTrigger : public Value<Event> {
public:
	void Start() { m_Ptr->Start(); }
	void Stop() { m_Ptr->Stop(); }
	bool IsStarted() { return m_Ptr->IsStarted(); }
	uint32_t GetTick() { return m_Ptr->GetTick(); }
	bool Trigger(bool c = true) { return m_Ptr->Trigger(c); }
	bool operator=(bool v) { SetState(v); return v; }
	void SetState(bool v) { if (v) Start(); else Stop(); }
};
typedef EventTrigger SwitchValue;


namespace ThemeHelper {
	void InitializeAll();
	void ReleaseAll();
}



/*
* I suggest to use macro in switch -
* as internal structure can be changed in any case.
*/
#define SWITCH_ON(s) (EVENTPOOL->Get(s)->Start())
#define SWITCH_OFF(s) (EVENTPOOL->Get(s)->Stop())
#define SWITCH_TRIGGER(s, cond) (EVENTPOOL->Trigger(s, cond))
#define SWITCH_GET(s) (EVENTPOOL->Get(s))

