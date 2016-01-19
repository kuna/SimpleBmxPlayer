#include "globalresources.h"
#include "file.h"
#include "util.h"

StringPool* STRPOOL = 0;
DoublePool* DOUBLEPOOL = 0;
IntPool* INTPOOL = 0;
TimerPool* TIMERPOOL = 0;
HandlerPool* HANDLERPOOL = 0;

ImagePool* IMAGEPOOL = 0;
FontPool* FONTPOOL = 0;
SoundPool* SOUNDPOOL = 0;

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

bool TimerPool::IsExists(const RString &key) {
	return _timerpool.find(key) != _timerpool.end();
}

Timer* TimerPool::Set(const RString &key, bool activate) {
	if (!IsExists(key)) {
		Timer t;
		if (activate)
			t.Start();
		else
			t.Stop();
		_timerpool.insert(std::pair<RString, Timer>(key, t));
	}
	else {
		if (activate)
			_timerpool[key].Start();
		else
			_timerpool[key].Stop();
	}
	return &_timerpool[key];
}

void TimerPool::Reset(const RString &key) {
	Set(key, true);
}

void TimerPool::Stop(const RString &key) {
	if (!IsExists(key)) return;
	_timerpool[key].Stop();
}

Timer* TimerPool::Get(const RString &key) {
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
	return _handlerpool.find(key) != _handlerpool.end();
}

_Handler HandlerPool::Add(const RString &key, _Handler h) {
	if (!IsExists(key)) {
		_handlerpool.insert(std::pair<RString, std::vector<_Handler>>(key, std::vector<_Handler>()));
	}
	_handlerpool[key].push_back(h);
	return h;
}

bool HandlerPool::Call(const RString &key, void* arg) {
	if (!IsExists(key)) return false;
	for (auto it = _handlerpool[key].begin(); it != _handlerpool[key].end(); ++it)
		(*it)(arg);
	return true;
}

bool HandlerPool::Remove(const RString &key, _Handler h) {
	if (!IsExists(key)) return false;
	for (auto it = _handlerpool[key].begin(); it != _handlerpool[key].end(); ++it) {
		if ((*it) == h) {
			_handlerpool[key].erase(it);
			return true;
		}
	}
	return false;
}

void HandlerPool::Clear() { _handlerpool.clear(); }

ImagePool::~ImagePool() { ReleaseAll(); }

void ImagePool::ReleaseAll() {
	for (auto it = _imagepool.begin(); it != _imagepool.end(); ++it) {
		delete it->second;
	}
	_imagepool.clear();
	_loadcount.clear();
}

bool ImagePool::Release(Image *img) {
	// reduce loadcount
	// if loadcount <= 0, then release object from memory
	if (_loadcount.find(img) != _loadcount.end()) {
		int loadcount = --_loadcount[img];
		if (loadcount <= 0) {
			for (auto it = _imagepool.begin(); it != _imagepool.end(); ++it) {
				if (it->second == img) {
					_imagepool.erase(it);
					break;
				}
			}
			_loadcount.erase(_loadcount.find(img));
			delete img;
		}
		return true;
	}
	else {
		// not exists
		return false;
	}
}

bool ImagePool::IsExists(const RString &path) {
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	return (_imagepool.find(_path) != _imagepool.end());
}

Image* ImagePool::Load(const RString &path) {
	// first convert path in easy way
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	if (!IsExists(_path)) {
		Image *img = new Image();
		// if filename == '*', then get any file in that directory
		if (IO::substitute_extension(IO::get_filename(_path), "") == "*") {
			RString _directory = IO::get_filedir(_path);
			std::vector<RString> filelist;
			FileHelper::GetFileList(_directory, filelist);
			if (filelist.size() > 0)
				_path = filelist[0];
		}
		img->Load(_path.c_str());
		if (img->IsLoaded()) {
			_imagepool.insert(pair<RString, Image*>(_path, img));
			_loadcount.insert(pair<Image*, int>(img, 1));
			return img;
		}
		else {
			delete img;
			return 0;
		}
	}
	else {
		Image *img = _imagepool[_path];
		_loadcount[img]++;
		return img;
	}
}

Image* ImagePool::Get(const RString &path) {
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	if (IsExists(_path)) {
		return _imagepool[_path];
	}
	else {
		return 0;
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
	_loadcount.clear();
}

bool FontPool::Release(Font *f) {
	// reduce loadcount
	// if loadcount <= 0, then release object from memory
	if (_loadcount.find(f) != _loadcount.end()) {
		int loadcount = --_loadcount[f];
		if (loadcount <= 0) {
			for (auto it = _fontpool.begin(); it != _fontpool.end(); ++it) {
				if (it->second == f) {
					_fontpool.erase(it);
					break;
				}
			}
			_loadcount.erase(_loadcount.find(f));
			delete f;
		}
		return true;
	}
	else {
		// not exists
		return false;
	}
}

bool FontPool::IsExists(const RString &path) {
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	return IsIDExists(_path);
}

bool FontPool::IsIDExists(const RString &id) {
	return (_fontpool.find(id) != _fontpool.end());
}

Font* FontPool::LoadTTFFont(const RString &path, 
	int size, SDL_Color color, int border, SDL_Color bordercolor,
	int style, int thickness, const char* texturepath) {
	return LoadTTFFont(path, path, size, color, border, bordercolor, style, thickness, texturepath);
}

Font* FontPool::LoadTTFFont(const RString& id, const RString &path, 
	int size, SDL_Color color, int border, SDL_Color bordercolor,
	int style, int thickness, const char* texturepath) {
	if (!IsExists(id)) {
		// first convert path in easy way
		RString _path = path;
		FileHelper::ConvertPathToAbsolute(_path);
		Font *f = new Font();
		// if filename == '*', then get any file in that directory
		if (IO::substitute_extension(IO::get_filename(_path), "") == "*") {
			RString _directory = IO::get_filedir(_path);
			std::vector<RString> filelist;
			FileHelper::GetFileList(_directory, filelist);
			if (filelist.size() > 0)
				_path = filelist[0];
		}
		f->LoadTTFFont(_path, size, color, border, bordercolor, style, thickness, texturepath);
		if (f->IsLoaded()) {
			_fontpool.insert(pair<RString, Font*>(id, f));
			_loadcount.insert(pair<Font*, int>(f, 1));
			return f;
		}
		else {
			delete f;
			return 0;
		}
	}
	else {
		Font *f = _fontpool[id];
		_loadcount[f]++;
		return f;
	}
}

Font* FontPool::Get(const RString &path) {
	RString _path = path;
	FileHelper::ConvertPathToAbsolute(_path);
	return GetByID(_path);
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
		if (IO::substitute_extension(IO::get_filename(_path), "") == "*") {
			RString _directory = IO::get_filedir(_path);
			std::vector<RString> filelist;
			FileHelper::GetFileList(_directory, filelist);
			if (filelist.size() > 0)
				_path = filelist[0];
		}
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
	TIMERPOOL = new TimerPool();
	HANDLERPOOL = new HandlerPool();
	IMAGEPOOL = new ImagePool();
	FONTPOOL = new FontPool();
	SOUNDPOOL = new SoundPool();
}

void PoolHelper::ReleaseAll() {
	delete STRPOOL;
	delete DOUBLEPOOL;
	delete INTPOOL;
	delete TIMERPOOL;
	delete HANDLERPOOL;
	delete IMAGEPOOL;
	delete FONTPOOL;
	delete SOUNDPOOL;
}