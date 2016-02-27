#include "Pool.h"
#include "file.h"
#include "util.h"
#include "game.h"
#include "logger.h"
using namespace Display;

StringPool* STRPOOL = 0;
DoublePool* DOUBLEPOOL = 0;
IntPool* INTPOOL = 0;
HandlerPool* HANDLERPOOL = 0;

TexturePool* TEXPOOL = 0;
FontPool* FONTPOOL = 0;
SoundPool* SOUNDPOOL = 0;





HandlerAuto::HandlerAuto() {
	ASSERT(HANDLERPOOL != 0);
	HANDLERPOOL->Register(this);
}

HandlerAuto::~HandlerAuto() {
	HANDLERPOOL->Remove(this);
}






bool StringPool::IsExists(const RString &key) {
	return _stringpool.find(key) != _stringpool.end();
}

RString* StringPool::Set(const RString &key, const RString &value) {
	if (!IsExists(key)) {
		_stringpool.insert(std::pair<RString, RString>(key, value));
	}
	else {
		_stringpool[key] = value;
	}
	return &_stringpool[key];
}

RString* StringPool::Get(const RString &key) {
	// create one if not exists
	if (!IsExists(key)) {
		Set(key);
	}
	return &_stringpool[key];
}

void StringPool::Clear() { _stringpool.clear(); }







bool DoublePool::IsExists(const RString &key) {
	return _doublepool.find(key) != _doublepool.end();
}

double* DoublePool::Set(const RString &key, double value) {
	if (!IsExists(key)) {
		_doublepool.insert(std::pair<RString, double>(key, value));
	}
	else {
		_doublepool[key] = value;
	}
	return &_doublepool[key];
}

double* DoublePool::Get(const RString &key) {
	// if no timer exists, then create as 0
	if (!IsExists(key)) {
		return Set(key, 0);
	}
	return &_doublepool[key];
}

void DoublePool::Clear() { _doublepool.clear(); }







bool IntPool::IsExists(const RString &key) {
	return _intpool.find(key) != _intpool.end();
}

int* IntPool::Set(const RString &key, int value) {
	if (!IsExists(key)) {
		_intpool.insert(std::pair<RString, int>(key, value));
	}
	else {
		_intpool[key] = value;
	}
	return &_intpool[key];
}

int* IntPool::Get(const RString &key) {
	// create one if not exists
	if (!IsExists(key)) {
		Set(key, 0);
	}
	return &_intpool[key];
}

void IntPool::Clear() { _intpool.clear(); }







bool HandlerPool::IsExists(const RString &key) {
	return _timerpool.find(key) != _timerpool.end();
}

void HandlerPool::Stop(const RString &key) {
	if (!IsExists(key)) return;
	_timerpool[key].Stop();
}

Timer* HandlerPool::Get(const RString &key) {
	// don't make null string timer
	if (!key.size())
		return 0;
	// if no timer exists, then create on as unknown State
	if (!IsExists(key)) {
		_timerpool.insert(std::pair<RString, Timer>(key, Timer(TIMERSTATUS::UNKNOWN)));
	}
	return &_timerpool[key];
}

bool HandlerPool::IsExists(const RString &key) {
	return _timerpool.find(key) != _timerpool.end();
}

void HandlerPool::Register(Handler* h) {
	_handlerpool.push_back(h);
}

bool HandlerPool::Reset(const RString &key) {
	// active handlers first, then timer.
	Message msg;
	msg.name = key;
	for (auto it = _handlerpool.begin(); it != _handlerpool.end(); ++it)
		(*it)->Receive(msg);
	_timerpool[key].Start();
	return true;
}

void HandlerPool::Remove(Handler* h) {
	for (auto it = _handlerpool.begin(); it != _handlerpool.end(); ++it) {
		if ((*it) == h) {
			_handlerpool.erase(it);
		}
	}
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
		if (loadcount <= 0) {
			for (auto it = _texpool.begin(); it != _texpool.end(); ++it) {
				if (it->second == tex) {
					_texpool.erase(it);
					break;
				}
			}
			// if related surface exists, then delete it
			if (_surface.find(tex) != _surface.end()) {
				delete _surface[tex];
				_surface.erase(tex);
			}
			// remove loadcount, of course
			_loadcount.erase(tex);
			delete tex;
		}
		return true;
	}
	else {
		// not exists
		return false;
	}
}

namespace {
	// private
	// if filename == '*', then get any file in that directory
	void GetAnyProperFile(RString &_path) {
		if (substitute_extension(get_filename(_path), "") == "*") {
			RString _directory = get_filedir(_path);
			std::vector<RString> filelist;
			FileHelper::GetFileList(_directory, filelist);
			if (filelist.size() > 0)
				_path = filelist[rand() % filelist.size()];
		}
	}
}

bool TexturePool::IsExists(const RString &path) {
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	return (_texpool.find(_path) != _texpool.end());
}

Texture* TexturePool::Load(const RString &path) {
	// POOL method searches image more smart way.
	// COMMENT: this uses basic Surface object, not SurfaceMovie object.
	//          If you want to load movie object, then use 
	RString _path = path;
	FileHelper::GetAnyAvailableFilePath(_path);
	if (!IsExists(_path)) {
		bool ismovie = false;
		Surface *surf = new Surface();
		if (!surf->Load(_path)) {
			delete surf;
			surf = new SurfaceMovie();
			if (!surf->Load(_path)) {
				delete surf;
				// cannot load texture
				LOG->Warn("Texture - Attempt to load surface, buf failed(%s).", path);
				return 0;
			}
			ismovie = true;
		}
		Texture *tex = DISPLAY->CreateTexture(surf);
		// if ismovie, then register surface into pool
		// to update texture later
		if (ismovie) {
			_surface[tex] = surf;
		}
		else delete surf;
		return tex;
	}
	else {
		Texture *tex = _texpool[_path];
		_loadcount[tex]++;
		return tex;
	}
}

Texture* TexturePool::Register(const RString& key, Texture *tex) {
	// only register available if no previous key exists
	if (IsExists(key)) {
		LOG->Warn("Texture - Attempt to overwrite texture in same key (ignored)");
	}
	else {
		_texpool[key] = tex;
		_loadcount[tex]++;
		return tex;
	}
}

void TexturePool::Update(Display::Texture* tex, Uint32 msec) {
	// ignore if no surface exists
	if (_surface.find(tex) != _surface.end()) {
		SurfaceMovie *surf = (SurfaceMovie*)_surface[tex];
		if (!surf) return;
		surf->UpdateSurface(msec);
		DISPLAY->UpdateTexture(tex, surf);
	}
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
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	GetAnyProperFile(_path);
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
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	return (_soundpool.find(_path) != _soundpool.end());
}

Audio* SoundPool::Load(const RString &path) {
	// first convert path in easy way
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	if (!IsExists(_path)) {
		// if filename == '*', then get any file in that directory
		GetAnyProperFile(_path);
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
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	if (IsExists(_path)) {
		return _soundpool[_path];
	}
	else {
		return 0;
	}
}

void PoolHelper::InitalizeAll() {
	STRPOOL = new StringPool();
	DOUBLEPOOL = new DoublePool();
	INTPOOL = new IntPool();
	HANDLERPOOL = new HandlerPool();
	TEXPOOL = new TexturePool();
	FONTPOOL = new FontPool();
	SOUNDPOOL = new SoundPool();
}

void PoolHelper::ReleaseAll() {
	SAFE_DELETE(STRPOOL);
	SAFE_DELETE(DOUBLEPOOL);
	SAFE_DELETE(INTPOOL);
	SAFE_DELETE(HANDLERPOOL);
	SAFE_DELETE(TEXPOOL);
	SAFE_DELETE(FONTPOOL);
	SAFE_DELETE(SOUNDPOOL);
}