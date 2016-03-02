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






Switch::Switch(const RString& name, int state) : m_Switchname(name), Timer(state) {}

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
	if (!tex) return;
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



SongValue			SONGVALUE;
PlayerRenderValue	PLAYERVALUE[4];

void Initalize_BmsValue() {
	SONGVALUE.songloadprogress = DOUBLEPOOL->Get("SongLoadProgress");
	SONGVALUE.OnSongLoading = SWITCH_GET("OnSongLoading");
	SONGVALUE.OnSongLoadingEnd = SWITCH_GET("OnSongLoadingEnd");

	SONGVALUE.PlayProgress = DOUBLEPOOL->Get("PlayProgress");
	SONGVALUE.PlayBPM = INTPOOL->Get("PlayBPM");
	SONGVALUE.PlayMin = INTPOOL->Get("PlayMinute");
	SONGVALUE.PlaySec = INTPOOL->Get("PlaySecond");
	SONGVALUE.PlayRemainSec = INTPOOL->Get("PlayRemainSecond");
	SONGVALUE.PlayRemainMin = INTPOOL->Get("PlayRemainMinute");

	SONGVALUE.OnBeat = SWITCH_GET("OnBeat");
	SONGVALUE.OnBgaMain = SWITCH_GET("OnBgaMain");
	SONGVALUE.OnBgaLayer1 = SWITCH_GET("OnBgaLayer1");
	SONGVALUE.OnBgaLayer2 = SWITCH_GET("OnBgaLayer2");
	SONGVALUE.SongTime = SWITCH_GET("OnGameStart");

	SONGVALUE.sMainTitle = STRPOOL->Get("MainTitle");
	SONGVALUE.sTitle = STRPOOL->Get("Title");
	SONGVALUE.sSubTitle = STRPOOL->Get("SubTitle");
	SONGVALUE.sGenre = STRPOOL->Get("Genre");
	SONGVALUE.sArtist = STRPOOL->Get("Artist");
	SONGVALUE.sSubArtist = STRPOOL->Get("SubArtist");
}

void Initalize_P1_RenderValue() {
	PLAYERVALUE[0].pNoteSpeed = INTPOOL->Get("P1Speed");
	PLAYERVALUE[0].pFloatSpeed = INTPOOL->Get("P1FloatSpeed");
	PLAYERVALUE[0].pSudden = INTPOOL->Get("P1Sudden");
	PLAYERVALUE[0].pLift = INTPOOL->Get("P1Lift");
	PLAYERVALUE[0].pSudden_d = DOUBLEPOOL->Get("P1Sudden");
	PLAYERVALUE[0].pLift_d = DOUBLEPOOL->Get("P1Lift");

	PLAYERVALUE[0].pGauge_d = DOUBLEPOOL->Get("P1Gauge");
	PLAYERVALUE[0].pGaugeType = INTPOOL->Get("P1GaugeType");
	PLAYERVALUE[0].pGauge = INTPOOL->Get("P1Gauge");
	PLAYERVALUE[0].pExscore = INTPOOL->Get("P1ExScore");
	PLAYERVALUE[0].pScore = INTPOOL->Get("P1Score");
	PLAYERVALUE[0].pExscore_d = DOUBLEPOOL->Get("P1ExScore");
	PLAYERVALUE[0].pHighscore_d = DOUBLEPOOL->Get("P1HighScore");
	PLAYERVALUE[0].pScore = INTPOOL->Get("P1Score");
	PLAYERVALUE[0].pCombo = INTPOOL->Get("P1Combo");
	PLAYERVALUE[0].pMaxCombo = INTPOOL->Get("P1MaxCombo");
	PLAYERVALUE[0].pTotalnotes = INTPOOL->Get("P1TotalNotes");
	PLAYERVALUE[0].pRivaldiff = INTPOOL->Get("P1RivalDiff");
	PLAYERVALUE[0].pRate = INTPOOL->Get("P1Rate");
	PLAYERVALUE[0].pTotalRate = INTPOOL->Get("P1TotalRate");
	PLAYERVALUE[0].pRate_d = DOUBLEPOOL->Get("P1Rate");
	PLAYERVALUE[0].pTotalRate_d = DOUBLEPOOL->Get("P1TotalRate");

	PLAYERVALUE[0].pOnJudge[5] = SWITCH_OFF("OnP1JudgePerfect");
	PLAYERVALUE[0].pOnJudge[4] = SWITCH_OFF("OnP1JudgeGreat");
	PLAYERVALUE[0].pOnJudge[3] = SWITCH_OFF("OnP1JudgeGood");
	PLAYERVALUE[0].pOnJudge[2] = SWITCH_OFF("OnP1JudgeBad");
	PLAYERVALUE[0].pOnJudge[1] = SWITCH_OFF("OnP1JudgePoor");
	PLAYERVALUE[0].pOnJudge[0] = SWITCH_OFF("OnP1JudgePoor");
	PLAYERVALUE[0].pNotePerfect = INTPOOL->Get("P1PerfectCount");
	PLAYERVALUE[0].pNoteGreat = INTPOOL->Get("P1GreatCount");
	PLAYERVALUE[0].pNoteGood = INTPOOL->Get("P1GoodCount");
	PLAYERVALUE[0].pNoteBad = INTPOOL->Get("P1BadCount");
	PLAYERVALUE[0].pNotePoor = INTPOOL->Get("P1PoorCount");
	PLAYERVALUE[0].pOnSlow = SWITCH_OFF("OnP1Slow");
	PLAYERVALUE[0].pOnFast = SWITCH_OFF("OnP1Fast");

	PLAYERVALUE[0].pOnAAA = SWITCH_GET("IsP1AAA");
	PLAYERVALUE[0].pOnAA = SWITCH_GET("IsP1AA");
	PLAYERVALUE[0].pOnA = SWITCH_GET("IsP1A");
	PLAYERVALUE[0].pOnB = SWITCH_GET("IsP1B");
	PLAYERVALUE[0].pOnC = SWITCH_GET("IsP1C");
	PLAYERVALUE[0].pOnD = SWITCH_GET("IsP1D");
	PLAYERVALUE[0].pOnE = SWITCH_GET("IsP1E");
	PLAYERVALUE[0].pOnF = SWITCH_GET("IsP1F");
	PLAYERVALUE[0].pOnReachAAA = SWITCH_GET("IsP1ReachAAA");
	PLAYERVALUE[0].pOnReachAA = SWITCH_GET("IsP1ReachAA");
	PLAYERVALUE[0].pOnReachA = SWITCH_GET("IsP1ReachA");
	PLAYERVALUE[0].pOnReachB = SWITCH_GET("IsP1ReachB");
	PLAYERVALUE[0].pOnReachC = SWITCH_GET("IsP1ReachC");
	PLAYERVALUE[0].pOnReachD = SWITCH_GET("IsP1ReachD");
	PLAYERVALUE[0].pOnReachE = SWITCH_GET("IsP1ReachE");
	PLAYERVALUE[0].pOnReachF = SWITCH_GET("IsP1ReachF");

	PLAYERVALUE[0].pOnMiss = SWITCH_GET("OnP1Miss");
	PLAYERVALUE[0].pOnCombo = SWITCH_GET("OnP1Combo");
	PLAYERVALUE[0].pOnfullcombo = SWITCH_GET("OnP1FullCombo");
	PLAYERVALUE[0].pOnlastnote = SWITCH_GET("OnP1LastNote");
	PLAYERVALUE[0].pOnGameover = SWITCH_GET("OnP1GameOver");
	PLAYERVALUE[0].pOnGaugeMax = SWITCH_GET("OnP1GaugeMax");
	PLAYERVALUE[0].pOnGaugeUp = SWITCH_GET("OnP1GaugeUp");

	/*
	* SC : note-index 0
	*/
	PLAYERVALUE[0].pLanepress[0] = SWITCH_GET("OnP1KeySCPress");
	PLAYERVALUE[0].pLaneup[0] = SWITCH_GET("OnP1KeySCUp");
	PLAYERVALUE[0].pLanehold[0] = SWITCH_GET("OnP1JudgeSCHold");
	PLAYERVALUE[0].pLanejudgeokay[0] = SWITCH_GET("OnP1JudgeSCOkay");
	for (int i = 1; i < 10; i++) {
		PLAYERVALUE[0].pLanepress[i] = SWITCH_GET(ssprintf("OnP1Key%dPress", i));
		PLAYERVALUE[0].pLaneup[i] = SWITCH_GET(ssprintf("OnP1Key%dUp", i));
		PLAYERVALUE[0].pLanehold[i] = SWITCH_GET(ssprintf("OnP1Judge%dHold", i));
		PLAYERVALUE[0].pLanejudgeokay[i] = SWITCH_GET(ssprintf("OnP1Judge%dOkay", i));
	}
}

void Initalize_P2_RenderValue() {
	PLAYERVALUE[1].pNoteSpeed = INTPOOL->Get("P2Speed");
	PLAYERVALUE[1].pFloatSpeed = INTPOOL->Get("P2FloatSpeed");
	PLAYERVALUE[1].pSudden = INTPOOL->Get("P2Sudden");
	PLAYERVALUE[1].pSudden_d = DOUBLEPOOL->Get("P2Sudden");
	PLAYERVALUE[1].pLift = INTPOOL->Get("P2Lift");
	PLAYERVALUE[1].pLift_d = DOUBLEPOOL->Get("P2Lift");

	PLAYERVALUE[1].pGauge_d = DOUBLEPOOL->Get("P2Gauge");
	PLAYERVALUE[1].pGaugeType = INTPOOL->Get("P2GaugeType");
	PLAYERVALUE[1].pGauge = INTPOOL->Get("P2Gauge");
	PLAYERVALUE[1].pExscore = INTPOOL->Get("P2ExScore");
	PLAYERVALUE[1].pScore = INTPOOL->Get("P2Score");
	PLAYERVALUE[1].pExscore_d = DOUBLEPOOL->Get("P2ExScore");
	PLAYERVALUE[1].pHighscore_d = DOUBLEPOOL->Get("P2HighScore");
	PLAYERVALUE[1].pScore = INTPOOL->Get("P2Score");
	PLAYERVALUE[1].pCombo = INTPOOL->Get("P2Combo");
	PLAYERVALUE[1].pMaxCombo = INTPOOL->Get("P2MaxCombo");
	PLAYERVALUE[1].pTotalnotes = INTPOOL->Get("P2TotalNotes");
	PLAYERVALUE[1].pRivaldiff = INTPOOL->Get("P2RivalDiff");
	PLAYERVALUE[1].pRate = INTPOOL->Get("P2Rate");
	PLAYERVALUE[1].pTotalRate = INTPOOL->Get("P2TotalRate");
	PLAYERVALUE[1].pRate_d = DOUBLEPOOL->Get("P2Rate");
	PLAYERVALUE[1].pTotalRate_d = DOUBLEPOOL->Get("P2TotalRate");

	PLAYERVALUE[1].pOnJudge[5] = SWITCH_OFF("OnP2JudgePerfect");
	PLAYERVALUE[1].pOnJudge[4] = SWITCH_OFF("OnP2JudgeGreat");
	PLAYERVALUE[1].pOnJudge[3] = SWITCH_OFF("OnP2JudgeGood");
	PLAYERVALUE[1].pOnJudge[2] = SWITCH_OFF("OnP2JudgeBad");
	PLAYERVALUE[1].pOnJudge[1] = SWITCH_OFF("OnP2JudgePoor");
	PLAYERVALUE[1].pOnJudge[0] = SWITCH_OFF("OnP2JudgePoor");
	PLAYERVALUE[1].pNotePerfect = INTPOOL->Get("P2PerfectCount");
	PLAYERVALUE[1].pNoteGreat = INTPOOL->Get("P2GreatCount");
	PLAYERVALUE[1].pNoteGood = INTPOOL->Get("P2GoodCount");
	PLAYERVALUE[1].pNoteBad = INTPOOL->Get("P2BadCount");
	PLAYERVALUE[1].pNotePoor = INTPOOL->Get("P2PoorCount");
	PLAYERVALUE[1].pOnSlow = SWITCH_OFF("OnP2Slow");
	PLAYERVALUE[1].pOnFast = SWITCH_OFF("OnP2Fast");

	PLAYERVALUE[1].pOnAAA = SWITCH_GET("IsP2AAA");
	PLAYERVALUE[1].pOnAA = SWITCH_GET("IsP2AA");
	PLAYERVALUE[1].pOnA = SWITCH_GET("IsP2A");
	PLAYERVALUE[1].pOnB = SWITCH_GET("IsP2B");
	PLAYERVALUE[1].pOnC = SWITCH_GET("IsP2C");
	PLAYERVALUE[1].pOnD = SWITCH_GET("IsP2D");
	PLAYERVALUE[1].pOnE = SWITCH_GET("IsP2E");
	PLAYERVALUE[1].pOnF = SWITCH_GET("IsP2F");
	PLAYERVALUE[1].pOnReachAAA = SWITCH_GET("IsP2ReachAAA");
	PLAYERVALUE[1].pOnReachAA = SWITCH_GET("IsP2ReachAA");
	PLAYERVALUE[1].pOnReachA = SWITCH_GET("IsP2ReachA");
	PLAYERVALUE[1].pOnReachB = SWITCH_GET("IsP2ReachB");
	PLAYERVALUE[1].pOnReachC = SWITCH_GET("IsP2ReachC");
	PLAYERVALUE[1].pOnReachD = SWITCH_GET("IsP2ReachD");
	PLAYERVALUE[1].pOnReachE = SWITCH_GET("IsP2ReachE");
	PLAYERVALUE[1].pOnReachF = SWITCH_GET("IsP2ReachF");

	PLAYERVALUE[1].pOnMiss = SWITCH_GET("OnP2Miss");
	PLAYERVALUE[1].pOnCombo = SWITCH_GET("OnP2Combo");
	PLAYERVALUE[1].pOnfullcombo = SWITCH_GET("OnP2FullCombo");
	PLAYERVALUE[1].pOnlastnote = SWITCH_GET("OnP2LastNote");
	PLAYERVALUE[1].pOnGameover = SWITCH_GET("OnP2GameOver");
	PLAYERVALUE[1].pOnGaugeMax = SWITCH_GET("OnP2GaugeMax");
	PLAYERVALUE[1].pOnGaugeUp = SWITCH_GET("OnP2GaugeUp");

	PLAYERVALUE[1].pLanepress[0] = SWITCH_GET("OnP2KeySCPress");
	PLAYERVALUE[1].pLaneup[0] = SWITCH_GET("OnP2KeySCUp");
	PLAYERVALUE[1].pLanehold[0] = SWITCH_GET("OnP2JudgeSCHold");
	PLAYERVALUE[1].pLanejudgeokay[0] = SWITCH_GET("OnP2JudgeSCOkay");
	for (int i = 1; i < 10; i++) {
		PLAYERVALUE[1].pLanepress[i] = SWITCH_GET(ssprintf("OnP2Key%dPress", i));
		PLAYERVALUE[1].pLaneup[i] = SWITCH_GET(ssprintf("OnP2Key%dUp", i));
		PLAYERVALUE[1].pLanehold[i] = SWITCH_GET(ssprintf("OnP2Judge%dHold", i));
		PLAYERVALUE[1].pLanejudgeokay[i] = SWITCH_GET(ssprintf("OnP2Judge%dOkay", i));
	}
}

/* private; automatically when PoolHelper::InitalizeAll() called */
void InitalizeValues() {
	Initalize_BmsValue();
	Initalize_P1_RenderValue();
	Initalize_P2_RenderValue();
}

void PoolHelper::InitalizeAll() {
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

void PoolHelper::ReleaseAll() {
	SAFE_DELETE(STRPOOL);
	SAFE_DELETE(DOUBLEPOOL);
	SAFE_DELETE(INTPOOL);
	SAFE_DELETE(HANDLERPOOL);
	SAFE_DELETE(TEXPOOL);
	SAFE_DELETE(FONTPOOL);
	SAFE_DELETE(SOUNDPOOL);
}