#include "globalresources.h"
#include "file.h"

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
	if (!IsExists(key))
		return 0;
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
	return &_doublepool[key];
}

double* DoublePool::Get(const RString &key) {
	if (!IsExists(key))
		return 0;
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
	return &_intpool[key];
}

int* IntPool::Get(const RString &key) {
	if (!IsExists(key))
		return 0;
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
	if (!IsExists(key)) return;
	_timerpool[key].Start();
}

void TimerPool::Stop(const RString &key) {
	if (!IsExists(key)) return;
	_timerpool[key].Stop();
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

Timer* TimerPool::Get(const RString &key) {
	if (!IsExists(key))
		return 0;
	return &_timerpool[key];
}

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
	// TODO
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
		// TODO
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