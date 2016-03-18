#include "Theme.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "skinconverter.h"
#include "ActorBasic.h"

#define SAFE_STRING(s) (s)?(s):"";

// actor registration
std::map<std::string, CreateActorFn> actormap;



void Theme::SetEnvironmentFromOption() {
	for (auto it = m_Skinmetric.values.begin(); it != m_Skinmetric.values.end(); ++it) {
		int i = 0;
		for (auto it2 = it->options.begin(); it2 != it->options.end(); ++it2) {
			bool trigger = false;
			int v = m_Option.values[it->optionname];
			if (i == v) {
				INTPOOL->Set(it->optionname.c_str(), v);
				trigger = true;
			}
			if (!it2->eventname.empty()) {
				Event* e = EVENTPOOL->Get(it2->eventname.c_str());
				if (trigger)
					e->Start();
				else
					e->Stop();
			}
		}
	}
	for (auto it = m_Skinmetric.files.begin(); it != m_Skinmetric.files.end(); ++it) {
		STRPOOL->Set(it->optionname.c_str(), m_Option.files[it->optionname]);
	}
}

void Theme::GetEnvironmentFromOption() {
	for (auto it = m_Skinmetric.values.begin(); it != m_Skinmetric.values.end(); ++it) {
		m_Option.values[it->optionname] = *INTPOOL->Get(it->optionname.c_str());
	}
	for (auto it = m_Skinmetric.files.begin(); it != m_Skinmetric.files.end(); ++it) {
		m_Option.files[it->optionname] = *STRPOOL->Get(it->optionname.c_str());
	}
}

// DEPRECIATED
#if 0
void Theme::DeleteEnvironmentFromOption() {
	for (auto it = m_Option.values.begin(); it != m_Option.values.end(); ++it) {
		INTPOOL->Remove(it->optionname.c_str());
	}
	for (auto it = m_Option.files.begin(); it != m_Option.files.end(); ++it) {
		STRPOOL->Remove(it->optionname.c_str());
	}
}
#endif

void Theme::LoadSkinOption() {
	// ../setting/themeoption.xml
	RString path = FILEMANAGER->GetAbsolutePath("../setting/themeoption.xml");
	XMLDocument doc(false);
	XMLElement *e_option;
	if (doc.LoadFile(path) == XML_SUCCESS || (e_option = doc.FirstChildElement("ThemeOption")) != 0) {
		for (XMLElement *e_theme = e_option->FirstChildElement("Theme");
			e_theme; e_theme = e_theme->NextSiblingElement("Theme"))
		{
			if (e_theme->Attribute("name", m_Skinmetric.info.title.c_str())) {
				for (XMLElement *e = e_theme->FirstChildElement("Option");
					e; e = e->NextSiblingElement("Option"))
				{
					m_Option.values[e->Attribute("key")] = e->IntAttribute("value");
				}
				for (XMLElement *e = e_theme->FirstChildElement("OptionFile");
					e; e = e->NextSiblingElement("OptionFile"))
				{
					m_Option.files[e->Attribute("key")] = e->Attribute("value");
				}
				return;
			}
		}
	}

	// cannot found? then make default value from metric
	for (auto it = m_Skinmetric.values.begin(); it != m_Skinmetric.values.end(); ++it) {
		m_Option.values[it->optionname] = 0;
	}
	for (auto it = m_Skinmetric.files.begin(); it != m_Skinmetric.files.end(); ++it) {
		m_Option.files[it->optionname] = it->path_default;
	}
	LOG->Warn("Cannot load Theme metrics from ../setting/themeoption.xml file. Set as default value.");
}

void Theme::SaveSkinOption() {
	RString path = FILEMANAGER->GetAbsolutePath("../setting/themeoption.xml");
	XMLDocument doc(false);

	// load previous themeoption.xml
	// if not exists, then make new one.
	XMLElement *e_option;
	if (doc.LoadFile(path) != XML_SUCCESS || (e_option = doc.FirstChildElement("ThemeOption")) == 0) {
		doc.Clear();
		e_option = doc.NewElement("ThemeOption");
		doc.LinkEndChild(e_option);
	}

	// if previous theme attribute exists, then remove it to write new one.
	XMLElement *e_theme;
	for (e_theme = doc.FirstChildElement("Theme");
		e_theme; )
	{
		XMLElement *_ = e_theme = e_theme->NextSiblingElement("Theme");
		if (e_theme->Attribute("name", m_Skinmetric.info.title.c_str())) {
			doc.DeleteNode(e_theme);
		}
		e_theme = _;
	}
	e_theme = doc.NewElement("Theme");
	e_theme->SetAttribute("name", m_Skinmetric.info.title.c_str());
	doc.LinkEndChild(e_theme);

	// write options
	for (auto it = m_Skinmetric.values.begin(); it != m_Skinmetric.values.end(); ++it) {
		XMLElement *e = doc.NewElement("Option");
		e->SetAttribute("key", it->optionname.c_str());
		e->SetAttribute("value", m_Option.values[it->optionname]);
		e_theme->LinkEndChild(e);
	}
	for (auto it = m_Skinmetric.files.begin(); it != m_Skinmetric.files.end(); ++it) {
		XMLElement *e = doc.NewElement("OptionFile");
		e->SetAttribute("key", it->optionname.c_str());
		e->SetAttribute("value", m_Option.files[it->optionname]);
		e_theme->LinkEndChild(e);
	}

	// save
	if (doc.SaveFile(path) != XML_SUCCESS) {
		LOG->Warn("Cannot write to ../setting/themeoption.xml file.");
	}
}




bool Theme::Load(const char* skinname) {
	// if previously theme loaded, then release it.
	ClearElements();

	// make metric path / skin path from skinname
	RString metricpath = FILEMANAGER->GetAbsolutePath(RString(skinname) + ".skin.xml");
	RString skinpath = FILEMANAGER->GetAbsolutePath(RString(skinname) + ".xml");		// can use xml, but better to use lua

	// load metric / skin from file
	// if file not exists, then attempt to convert lr2skin.
	if (!FileHelper::IsFile(metricpath)) {
		RString lr2skinpath = FILEMANAGER->GetAbsolutePath(RString(skinname) + ".lr2skin");
		if (!SkinConverter::ConvertLR2SkinToLua(lr2skinpath)) {
			LOG->Warn("Theme - Cannot convert/find LR2Skin to Lua (%s)", lr2skinpath);
			return false;
		}
	}
	if (!m_Skinmetric.Load(metricpath)) {
		LOG->Warn("Theme - Cannot load skin metrics (%s)", metricpath);
		return false;
	}
	if (!m_Skin.Load(skinpath)) {
		LOG->Warn("Theme - Cannot load skin structure (%s)", skinpath);
		return false;
	}

	// load(make) options and set switch
	LoadSkinOption();
	SetEnvironmentFromOption();

	// make rendering tree from Skin element
	base_ = MakeActor(m_Skin.GetBaseElement());

	return true;
}

void Theme::ClearElements() {
	// before release rendertree and else, 
	// reset options from skinoption
	GetEnvironmentFromOption();
	SaveSkinOption();
	//m_Skinoption.DeleteEnvironmentFromOption();

	// release render trees
	// (will do automatically recursive)
	delete base_;

	// clear all
	base_ = 0;
	m_Skin.Clear();
	m_Skinmetric.Clear();
	m_Option.values.clear();
	m_Option.files.clear();

	// We won't release skin resource (It'll be soon reused, actually)
	// (But it should be done manually if you change or reload skin)
}

void Theme::Update() { 
	// TODO use tick time
	if (base_) base_->Update();
}

void Theme::Render() {
	if (base_) {
		// before start rendering,
		// set screen ratio to skin is fully drawn to screen
		// (TODO)

		// render
		base_->Render();
	}
}

// MakeActor
Actor* Theme::MakeActor(const XMLElement *e, Actor* parent) {
	// type include: search for another
	// (DEPRECIATED; only for XML file)
	const char* name = e->Name();
	Actor* pActor = 0;

	// frame, sprite, number, text, notefield, judge
	auto iter = actormap.find(name);
	if (iter == actormap.end()) {
		LOG->Warn("ActorCreator - Undefined element [%s], ignore.", name);
	}
	else {
		pActor = actormap[name]();
		pActor->SetParent(parent);
		pActor->SetFromXml(e);
	}
	return pActor;
}

void Theme::RegisterActor(const std::string& name, CreateActorFn fn) {
	auto iter = actormap.find(name);
	if (iter != actormap.end()) {
		LOG->Warn("RegisterActor - Duplicate Actor already registered (%s), ignored.", name.c_str());
	}
	else actormap[name] = fn;
}




// lua related
// TODO: LoadImage, LoadFont
// Make these array as singleton, as lua will act like running in single thread

std::map<std::string, Display::Texture*> lua_texmap;
std::map<std::string, Font*> lua_fontmap;

// input: table(path:string, key:string)
// output: bool
int Lua_LoadImage(Lua* L) {
	if (!lua_istable(L, -1)) {
		LOG->Warn("Lua_LoadImage: invalid argument - table should given as argument.");
		lua_pop(L, 1);
		RETURN_FALSE_LUA;
	}

	std::string path, key;
	GETKEY_LUA("path");
	if (!lua_isstring(L, -1)) {
		LOG->Warn("Lua_LoadImage: invalid argument - path isn't string.");
		lua_pop(L, 2);
		RETURN_FALSE_LUA;
	}
	path = lua_tostring(L, -1);
	lua_pop(L, 1);

	GETKEY_LUA("id");
	if (!lua_isstring(L, -1)) {
		LOG->Warn("Lua_LoadImage: invalid argument - id isn't string.");
		lua_pop(L, 2);
		RETURN_FALSE_LUA;
	}
	key = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_texmap[key] = TEXPOOL->Load(path);
	lua_pop(L, 1);
	RETURN_TRUE_LUA;
}

// input: table(path:string, color:string(RGB), id:string)
//        table(raw:string, id:string)
//
// if path ends with .ttf, then attempt to load with TTF font
// else, then load as bitmap font.
int Lua_LoadFont(Lua* L) {
	if (!lua_istable(L, -1)) {
		LOG->Warn("Lua_LoadFont: invalid argument - table should given as argument.");
		lua_pop(L, 1);
		RETURN_FALSE_LUA;
	}

	std::string path, id, raw, color, bordercolor;
	int thick = 0, size = 10, border = 0;
	Font *f;

	GETKEY_LUA("id");
	if (!lua_isstring(L, -1)) {
		LOG->Warn("Lua_LoadFont: invalid argument - id isn't string.");
		lua_pop(L, 2);
		RETURN_FALSE_LUA;
	}
	id = lua_tostring(L, -1);
	lua_pop(L, 1);

	// if raw exists, then directly attempt to load font
	GETKEY_LUA("raw");
	if (lua_isstring(L, -1)) {
		raw = lua_tostring(L, -1);
		lua_pop(L, 2);
		// COMMENT: Bit of problem - raw texture has no path, so it cannot be distinguished.
		//          So I have to hash raw, and use that code as identifying code(= path).
		if (f = FONTPOOL->LoadTextureFontFromTextData(id.c_str(), raw)) {
			lua_fontmap[id] = f;
			RETURN_TRUE_LUA;
		}
		else {
			RETURN_FALSE_LUA;
		}
	}
	lua_pop(L, 1);


	GETKEY_LUA("path");
	if (!lua_isstring(L, -1)) {
		LOG->Warn("Lua_LoadFont: invalid argument - path isn't string.");
		lua_pop(L, 2);
		RETURN_FALSE_LUA;
	}
	path = lua_tostring(L, -1);
	lua_pop(L, 1);


	GETKEY_LUA("color");
	if (lua_isstring(L, -1)) {
		color = lua_tostring(L, -1);
	}
	lua_pop(L, 1);

	GETKEY_LUA("bordercolor");
	if (lua_isstring(L, -1)) {
		bordercolor = lua_tostring(L, -1);
	}
	lua_pop(L, 1);

	GETKEY_LUA("thick");
	if (lua_isnumber(L, -1)) {
		thick = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	GETKEY_LUA("size");
	if (lua_isnumber(L, -1)) {
		size = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	GETKEY_LUA("border");
	if (lua_isnumber(L, -1)) {
		border = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	// if path ends with .ttf, load as ttf
	// otherwise, load as bitmap font
	lua_pop(L, 1);	// pop table
	if (EndsWith(path, ".ttf")) {
		if (f = FONTPOOL->LoadTTFFont(id.c_str(), path, size, 
			MakeRGBAInt(color), thick, MakeRGBAInt(bordercolor), border))
		{
			lua_fontmap[id] = f;
			RETURN_TRUE_LUA;
		}
		else {
			RETURN_FALSE_LUA;
		}
	}
	else {
		if (f = FONTPOOL->LoadTextureFont(id.c_str(), path))
		{
			lua_fontmap[id] = f;
			RETURN_TRUE_LUA;
		}
		else {
			RETURN_FALSE_LUA;
		}
	}
}

#include "LuaHelper.h"
void LuaBinding<Theme>::Register(Lua *L, int iMethods, int iMetatable) {
	luabridge::getGlobalNamespace(L)
		.addCFunction("LoadImage", Lua_LoadImage)
		.addCFunction("LoadFont", Lua_LoadFont);
}


void Theme::ClearAllResource() {
	lua_texmap.clear();
	lua_fontmap.clear();
}

Display::Texture* Theme::GetTexture(const std::string id) {
	auto iter = lua_texmap.find(id);
	if (iter == lua_texmap.end()) return 0;
	else return iter->second;
}

Font* Theme::GetFont(const std::string id) {
	auto iter = lua_fontmap.find(id);
	if (iter == lua_fontmap.end()) return 0;
	else return iter->second;
}



// --------------------------------------------------------------

#include "Display.h"
using namespace Display;

BasicPool<RString>* STRPOOL = 0;
BasicPool<double>* DOUBLEPOOL = 0;
BasicPool<int>* INTPOOL = 0;
EventPool* EVENTPOOL = 0;

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

Value<Event>& Value<Event>::SetFromPool(const RString& name) {
	m_Ptr = EVENTPOOL->Get(name);
	return *this;
}

Value<RString>& Value<RString>::SetFromPool(int player, const RString& name) {
	return SetFromPool(ssprintf("P%d", player) + name);
}

Value<int>& Value<int>::SetFromPool(int player, const RString& name) {
	return SetFromPool(ssprintf("P%d", player) + name);
}

Value<double>& Value<double>::SetFromPool(int player, const RString& name) {
	return SetFromPool(ssprintf("P%d", player) + name);
}

Value<Event>& Value<Event>::SetFromPool(int player, const RString& name) {
	return SetFromPool(ssprintf("P%d", player) + name);
}



void Start();			// always trigger, regardless of current status
void Stop();
bool Trigger();			// only start when timer isn't started
void Pause();
bool IsStarted(const RString& name);

void Register(Handler* h);
void UnRegister(Handler *h);

uint32_t GetTick();




Event::Event(const RString& name) : eventname(name) {}

void Event::Start() {
	// active handlers first, then timer.
	for (auto it = Handler_Begin(); it != Handler_End(); ++it) {
		(*it)->Trigger(eventname);
	}
	timer.Start();
}

void Event::Stop() {
	timer.Stop();
}

bool Event::Trigger(bool condition) {
	if (!IsStarted() && condition) {
		Start();
		return true;
	}
	else {
		return false;
	}
}

void Event::Pause() {
	timer.Pause();
}

bool Event::IsStarted() {
	return timer.IsStarted();
}

void Event::Register(Handler *h) {
	// COMMENT: better to check duplication to prevent any exception
	handlers.push_back(h);
}

void Event::UnRegister(Handler *h) {
	for (auto it = Handler_Begin(); it != Handler_End(); ++it) {
		if ((*it) == h) {
			handlers.erase(it);
			break;
		}
	}
}

uint32_t Event::GetTick() { return timer.GetTick(); }

void Event::SetArgument(int idx, const RString& arg) { EVENTPOOL->SetArgument(idx, arg); }

RString Event::GetArgument(int idx) { return EVENTPOOL->GetArgument(idx); }


Event* EventPool::Get(const RString &key) {
	// don't make null string timer
	if (!key.size())
		return 0;
	// if no timer exists, then create on as unknown State
	if (!IsExists(key)) {
		_eventpool.insert(std::pair<RString, Event>(key, Event(key)));
	}
	return &_eventpool[key];
}

bool EventPool::IsExists(const RString &key) {
	return _eventpool.find(key) != _eventpool.end();
}

void EventPool::Register(const RString& key, Handler* h) {
	if (!h) return;
	_eventpool[key].Register(h);
}

void EventPool::UnRegister(const RString& key, Handler* h) {
	auto it = _eventpool.find(key);
	if (it == _eventpool.end()) {
		LOG->Warn("Undefined handler %s", key.c_str());
		return;
	}
	it->second.UnRegister(h);
}

void EventPool::UnRegisterAll(Handler* h) {
	for (auto it = _eventpool.begin(); it != _eventpool.end(); ++it) {
		it->second.UnRegister(h);
	}
}

void EventPool::Remove(const RString& key) {
	_eventpool.erase(key);
}

void EventPool::Clear() {
	_eventpool.clear();
}

void EventPool::SetArgument(int idx, const RString& arg) {
	argv[idx] = arg;
}

RString EventPool::GetArgument(int idx) {
	return argv[idx];
}





TexturePool::~TexturePool() { ReleaseAll(); }

void TexturePool::ReleaseAll() {
	for (auto it = _texpool.begin(); it != _texpool.end(); ++it) {
		DISPLAY->DeleteTexture(it->second);
	}
	_texpool.clear();
	_loadcount.clear();
}

bool TexturePool::Release(Texture *tex) {
	if (!tex) return true;
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



/* private; automatically when PoolHelper::InitalizeAll() called */
void InitalizeLua();
void InitalizeValues() {
	GameTimer::Tick();		// do base tick

}
namespace ThemeHelper {
	void InitializeAll() {
		STRPOOL = new BasicPool<RString>();
		DOUBLEPOOL = new BasicPool<double>();
		INTPOOL = new BasicPool<int>();
		EVENTPOOL = new EventPool();
		TEXPOOL = new TexturePool();
		FONTPOOL = new FontPool();
		SOUNDPOOL = new SoundPool();

		// fill all basic values
		InitalizeValues();
		InitalizeLua();
	}

	void ReleaseAll() {
		SAFE_DELETE(STRPOOL);
		SAFE_DELETE(DOUBLEPOOL);
		SAFE_DELETE(INTPOOL);
		SAFE_DELETE(EVENTPOOL);
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
		SWITCH_GET(key)->Stop();
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
		Timer* t = SWITCH_GET(key)->GetTimer();
		if (t && t->IsStarted())
			lua_pushboolean(l, 1);
		else
			lua_pushboolean(l, 0);
		return 1;
	}

	int GetTime(Lua *l) {
		RString key = (const char*)lua_tostring(l, -1);
		lua_pop(l, 1);
		Timer *t = SWITCH_GET(key)->GetTimer();
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

void InitalizeLua() {
	Lua *L = LUA->Get();

	// register to lua
	lua_register(L, "SetString", SetString);
	lua_register(L, "GetString", GetString);

	lua_register(L, "SetHandler", SetTimer);
	lua_register(L, "SetSwitch", SetTimer);
	lua_register(L, "IsSwitch", IsTimer);
	lua_register(L, "GetTime", GetTime);

	lua_register(L, "SetInt", SetInt);
	lua_register(L, "GetInt", GetInt);

	lua_register(L, "SetFloat", SetFloat);
	lua_register(L, "GetFloat", GetFloat);

	LUA->Release(L);
}
