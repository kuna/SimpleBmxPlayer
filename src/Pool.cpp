#include "Pool.h"
#include "file.h"
#include "util.h"
#include "game.h"
#include "logger.h"
using namespace Display;

BasicPool<RString>* STRPOOL = 0;
BasicPool<double>* DOUBLEPOOL = 0;
BasicPool<int>* INTPOOL = 0;
SwitchValue* HANDLERPOOL = 0;

TexturePool* TEXPOOL = 0;
FontPool* FONTPOOL = 0;
SoundPool* SOUNDPOOL = 0;




/*
 * general pool/value setting
 */

Value<RString>& Value<RString>::SetFromPool(const RString& name) {
	m_Ptr = STRPOOL->Get(name);
	return *this;
}

Value<int>& Value<int>::SetFromPool(const RString& name) {
	m_Ptr = INTPOOL->Get(name);
	return *this;
}

Value<double>& Value<double>::SetFromPool(const RString& name) {
	m_Ptr = DOUBLEPOOL->Get(name);
	return *this;
}

PlayerValue<RString>& PlayerValue<RString>::SetFromPool(int player, const RString& name) {
	m_Ptr = STRPOOL->Get(name);
	return *this;
}

PlayerValue<int>& PlayerValue<int>::SetFromPool(int player, const RString& name) {
	m_Ptr = INTPOOL->Get(ssprintf("P%d", player) + name);
	return *this;
}

PlayerValue<double>& PlayerValue<double>::SetFromPool(int player, const RString& name) {
	m_Ptr = DOUBLEPOOL->Get(name);
	return *this;
}

void SwitchValue::Start() {
	m_Ptr->Start();
}

void SwitchValue::Stop() {
	m_Ptr->Stop();
}

bool SwitchValue::Trigger(bool c) {
	return m_Ptr->Trigger(c);
}

void PlayerSwitchValue::Start() {
	m_Ptr->Start();
}

void PlayerSwitchValue::Stop() {
	m_Ptr->Stop();
}

bool PlayerSwitchValue::Trigger(bool c) {
	return m_Ptr->Trigger(c);
}





HandlerAuto::HandlerAuto() {
	ASSERT(HANDLERPOOL != 0);
	HANDLERPOOL->Register(this);
}

HandlerAuto::~HandlerAuto() {
	HANDLERPOOL->RemoveHandler(this);
}





Switch::Switch(const RString& name, int state) : m_Switchname(name), Timer(state) {
	// noname isn't accepted
	ASSERT(!m_Switchname.empty());
}

void Switch::Start() {
	// active handlers first, then timer.
	HANDLERPOOL->CallHandler(m_Switchname);
	Timer::Start();
}

void Switch::Stop() {
	// nothing special ...
	Timer::Stop();
}

// redeclaration for rewriting Timer::Start() function 
bool Switch::Trigger(bool condition) {
	if (!IsStarted() && condition) {
		Start();
		return true;
	}
	else {
		return false;
	}
}

bool HandlerPool::IsExists(const RString &key) {
	return _timerpool.find(key) != _timerpool.end();
}

void HandlerPool::Register(Handler* h) {
	if (!h) return;
	_handlerpool.push_back(h);
}

void HandlerPool::CallHandler(const RString &key) {
	Message msg;
	msg.name = key;
	for (auto it = _handlerpool.begin(); it != _handlerpool.end(); ++it)
		(*it)->Receive(msg);
}

Switch* HandlerPool::Get(const RString &key) {
	// don't make null string timer
	if (!key.size())
		return 0;
	// if no timer exists, then create on as unknown State
	if (!IsExists(key)) {
		_timerpool.insert(std::pair<RString, Switch>(key, Switch(key)));
	}
	return &_timerpool[key];
}

Switch* HandlerPool::Start(const RString &key) {
	_timerpool[key].Start();
	return &_timerpool[key];
}

Switch* HandlerPool::Stop(const RString &key) {
	//if (!IsExists(key)) return;
	_timerpool[key].Stop();
	return &_timerpool[key];
}

void HandlerPool::RemoveHandler(Handler* h) {
	for (auto it = _handlerpool.begin(); it != _handlerpool.end(); ++it) {
		if ((*it) == h) {
			_handlerpool.erase(it);
		}
	}
}

void HandlerPool::Remove(const RString& key) {
	_timerpool.erase(key);
}

void HandlerPool::Clear() { _handlerpool.clear(); }







TexturePool::~TexturePool() { ReleaseAll(); }

void TexturePool::ReleaseAll() {
	for (auto it = _texpool.begin(); it != _texpool.end(); ++it) {
		DISPLAY->DeleteTexture(it->second);
	}
	_texpool.clear();
	_loadcount.clear();
}

bool TexturePool::Release(Texture *tex) {
	// reduce loadcount
	// if loadcount <= 0, then release object from memory
	if (_loadcount.find(tex) != _loadcount.end()) {
		int loadcount = --_loadcount[tex];
		// if loadcount is zero,
		// release texture && delete from pool.
		if (loadcount <= 0) {
			for (auto it = _texpool.begin(); it != _texpool.end(); ++it) {
				if (it->second == tex) {
					_texpool.erase(it);
					break;
				}
			}
			// remove loadcount, of course
			_loadcount.erase(tex);
			DISPLAY->DeleteTexture(tex);
		}
		return true;
	}
	else {
		// not exists
		return false;
	}
}

bool TexturePool::IsExists(const RString &path) {
	RString _path = path; //FILEMANAGER->GetAbsolutePath(path);
	FileHelper::GetAnyAvailableFilePath(_path);
	return (_texpool.find(_path) != _texpool.end());
}

Texture* TexturePool::Load(const RString &path) {
	// POOL method searches image more smart way.
	// COMMENT: this uses basic Surface object, not SurfaceMovie object.
	//          If you want to load movie object, then use 
	RString _path = path;
	FileHelper::GetAnyAvailableFilePath(_path);
	if (!IsExists(_path)) {
		Texture* tex = SurfaceUtil::LoadTexture(_path);
		if (!tex) return 0;
		else Register(_path, tex);
		return tex;
	}
	else {
		Texture *tex = _texpool[_path];
		_loadcount[tex]++;
		return tex;
	}
}

Texture* TexturePool::Register(const RString& key, Texture *tex) {
	if (!tex) return 0;
	// only register available if no previous key exists
	if (IsExists(key)) {
		LOG->Warn("Texture - Attempt to overwrite texture in same key (ignored)");
	}
	else {
		_texpool[key] = tex;
		_loadcount[tex]++;
	}
	return tex;
}

Texture* TexturePool::Get(const RString &path) {
	RString _path = path;
	// TODO: this costs a lot. better to change it into ID attribute?
	FileHelper::GetAnyAvailableFilePath(_path);
	auto iter = _texpool.find(_path);
	if (iter == _texpool.end()) {
		return Load(_path);
	}
	else {
		return iter->second;
	}
}







FontPool::~FontPool() {
	ReleaseAll();
}

void FontPool::ReleaseAll() {
	for (auto it = _fontpool.begin(); it != _fontpool.end(); ++it) {
		delete it->second;
	}
	_fontpool.clear();
}

bool FontPool::Release(Font *f) {
	for (auto it = _fontpool.begin(); it != _fontpool.end(); ++it) {
		if (it->second == f) {
			_fontpool.erase(it);
			delete f;
			return true;
		}
	}
	return false;
}

/* private */
RString randomID() { return ssprintf("%08x", rand()); }

bool FontPool::IsIDExists(const RString &id) {
	return (_fontpool.find(id) != _fontpool.end());
}

Font* FontPool::RegisterFont(const char* id, Font* f) {
	RString id_;
	if (!id) id_ = randomID();
	else id_ = id;
	if (!IsIDExists(id)) {
		_fontpool.insert(pair<RString, Font*>(id, f));
		return f;
	}
	else {
		// you can't register already existing font!
		return 0;
	}
}

Font* FontPool::LoadTTFFont(const char* id, const RString &path, 
	int size, Uint32 color, int border, Uint32 bordercolor,
	int style, int thickness, const char* texturepath) {
#if 0
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	GetAnyProperFile(_path);
	Font *f = new Font();
	f->LoadTTFFont(_path, size, color, border, bordercolor, style, thickness, texturepath);
	if (f->IsLoaded() && RegisterFont(id, f)) {
		return f;
	}
	else {
		delete f;
		return 0;
	}
#endif
	return 0;
}

Font* FontPool::LoadTextureFont(const char* id, const RString &path) {
	// first convert path in easy way
	RString _path = FILEMANAGER->GetAbsolutePath(path);
	RString content;
	if (GetFileContents(_path, content)) {
		return LoadTextureFontFromTextData(_path, content);
	}
	else {
		return 0;
	}
}

Font* FontPool::LoadTextureFontFromTextData(const char* id, const RString &textdata) {
	Font *f = new Font();
	f->LoadBitmapFont(textdata);
	if (RegisterFont(id, f)) {
		return f;
	}
	else {
		delete f;
		return 0;
	}
}

Font* FontPool::GetByID(const RString &id) {
	if (IsIDExists(id)) {
		return _fontpool[id];
	}
	else {
		return 0;
	}
}







SoundPool::~SoundPool() {
	ReleaseAll();
}

void SoundPool::ReleaseAll() {
	for (auto it = _soundpool.begin(); it != _soundpool.end(); ++it) {
		delete it->second;
	}
	_soundpool.clear();
	_loadcount.clear();
}

bool SoundPool::Release(Audio *audio) {
	// reduce loadcount
	// if loadcount <= 0, then release object from memory
	if (_loadcount.find(audio) != _loadcount.end()) {
		int loadcount = --_loadcount[audio];
		if (loadcount <= 0) {
			for (auto it = _soundpool.begin(); it != _soundpool.end(); ++it) {
				if (it->second == audio) {
					_soundpool.erase(it);
					break;
				}
			}
			_loadcount.erase(_loadcount.find(audio));
			delete audio;
		}
		return true;
	}
	else {
		// not exists
		return false;
	}
}

bool SoundPool::IsExists(const RString &path) {
	RString _path = FILEMANAGER->GetAbsolutePath(path);
	return (_soundpool.find(_path) != _soundpool.end());
}

Audio* SoundPool::Load(const RString &path) {
	// first convert path in easy way
	RString _path = FILEMANAGER->GetAbsolutePath(path);
	if (!IsExists(_path)) {
		Audio *audio = new Audio(_path);
		_soundpool.insert(pair<RString, Audio*>(_path, audio));
		_loadcount.insert(pair<Audio*, int>(audio, 1));
		return audio;
	}
	else {
		Audio *audio = _soundpool[_path];
		_loadcount[audio]++;
		return audio;
	}
}

Audio* SoundPool::Get(const RString &path) {
	RString _path = FILEMANAGER->GetAbsolutePath(path);
	if (IsExists(_path)) {
		return _soundpool[_path];
	}
	else {
		return 0;
	}
}



SongValue			SONGVALUE;

void Initalize_BmsValue() {
	SONGVALUE.songloadprogress = DOUBLEPOOL->Get("SongLoadProgress");
	SONGVALUE.OnSongLoading = SWITCH_GET("SongLoading");
	SONGVALUE.OnSongLoadingEnd = SWITCH_GET("SongLoadingEnd");

	SONGVALUE.PlayProgress = DOUBLEPOOL->Get("PlayProgress");
	SONGVALUE.PlayBPM = INTPOOL->Get("PlayBPM");
	SONGVALUE.PlayMin = INTPOOL->Get("PlayMinute");
	SONGVALUE.PlaySec = INTPOOL->Get("PlaySecond");
	SONGVALUE.PlayRemainSec = INTPOOL->Get("PlayRemainSecond");
	SONGVALUE.PlayRemainMin = INTPOOL->Get("PlayRemainMinute");

	SONGVALUE.OnBeat = SWITCH_GET("Beat");
	SONGVALUE.OnBgaMain = SWITCH_GET("BgaMain");
	SONGVALUE.OnBgaLayer1 = SWITCH_GET("BgaLayer1");
	SONGVALUE.OnBgaLayer2 = SWITCH_GET("BgaLayer2");
	SONGVALUE.SongTime = SWITCH_GET("GameStart");

	SONGVALUE.sMainTitle = STRPOOL->Get("MainTitle");
	SONGVALUE.sTitle = STRPOOL->Get("Title");
	SONGVALUE.sSubTitle = STRPOOL->Get("SubTitle");
	SONGVALUE.sGenre = STRPOOL->Get("Genre");
	SONGVALUE.sArtist = STRPOOL->Get("Artist");
	SONGVALUE.sSubArtist = STRPOOL->Get("SubArtist");
}

void Initalize_SceneValue() {
	SCENEVALUE.Uptime = SWITCH_GET("Game");
	SCENEVALUE.Scenetime = SWITCH_GET("Scene");
	SCENEVALUE.Rendertime = SWITCH_GET("Render");
}

/* private; automatically when PoolHelper::InitalizeAll() called */
void InitalizeValues() {
	GameTimer::Tick();		// do base tick
	Initalize_BmsValue();
}

namespace PoolHelper {
	void InitalizeAll() {
		STRPOOL = new StringPool();
		DOUBLEPOOL = new DoublePool();
		INTPOOL = new IntPool();
		HANDLERPOOL = new HandlerPool();
		TEXPOOL = new TexturePool();
		FONTPOOL = new FontPool();
		SOUNDPOOL = new SoundPool();

		// fill all basic values
		InitalizeValues();
	}

	void ReleaseAll() {
		SAFE_DELETE(STRPOOL);
		SAFE_DELETE(DOUBLEPOOL);
		SAFE_DELETE(INTPOOL);
		SAFE_DELETE(HANDLERPOOL);
		SAFE_DELETE(TEXPOOL);
		SAFE_DELETE(FONTPOOL);
		SAFE_DELETE(SOUNDPOOL);
	}
}


// lua part
#include "Luamanager.h"

/*
 * registering basic lua function start
 * TODO: generating rendering object (after we're done enough)
 */
namespace {
	int SetTimer(Lua* l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		SWITCH_OFF(key);
		//lua_pushinteger(l, a + 1);
		// no return value
		return 0;
	}

	int SetInt(Lua* l) {
		int val = (int)lua_tointeger(l, -1);
		lua_pop(l, 1);
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		INTPOOL->Set(key, val);
		return 0;	// no return value
	}

	int SetFloat(Lua* l) {
		int val = (int)lua_tointeger(l, -1);
		lua_pop(l, 1);
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		INTPOOL->Set(key, val);
		return 0;	// no return value
	}

	int SetString(Lua* l) {
		int val = (int)lua_tointeger(l, -1);
		lua_pop(l, 1);
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		INTPOOL->Set(key, val);
		return 0;	// no return value
	}

	int IsTimer(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		Timer* t = SWITCH_GET(key);
		if (t && t->IsStarted())
			lua_pushboolean(l, 1);
		else
			lua_pushboolean(l, 0);
		return 1;
	}

	int GetTime(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		Timer *t = SWITCH_GET(key);
		if (t)
			lua_pushinteger(l, t->GetTick());
		else
			lua_pushinteger(l, 0);
		return 1;
	}

	int GetInt(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		int *v = INTPOOL->Get(key);
		if (v)
			lua_pushinteger(l, *v);
		else
			lua_pushinteger(l, 0);
		return 1;
	}

	int GetFloat(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		double *d = DOUBLEPOOL->Get(key);
		if (d)
			lua_pushnumber(l, *d);
		else
			lua_pushnumber(l, 0);
		return 1;
	}

	int GetString(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		RString *s = STRPOOL->Get(key);
		if (s)
			lua_pushstring(l, *s);
		else
			lua_pushstring(l, "");
		return 1;
	}
}

void LuaBinding<StringPool>::Register(Lua *L, int iMethods, int iMetatable) {
	lua_register(L, "SetString", SetString);
	lua_register(L, "GetString", GetString);
}

void LuaBinding<HandlerPool>::Register(Lua *L, int iMethods, int iMetatable) {
	lua_register(L, "SetHandler", SetTimer);
	lua_register(L, "SetSwitch", SetTimer);
	lua_register(L, "IsSwitch", IsTimer);
	lua_register(L, "GetTime", GetTime);
}

void LuaBinding<IntPool>::Register(Lua *L, int iMethods, int iMetatable) {
	lua_register(L, "SetInt", SetInt);
	lua_register(L, "GetInt", GetInt);
}

void LuaBinding<DoublePool>::Register(Lua *L, int iMethods, int iMetatable) {
	lua_register(L, "SetFloat", SetFloat);
	lua_register(L, "GetFloat", GetFloat);
}