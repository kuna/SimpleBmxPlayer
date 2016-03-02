#include "Theme.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "skinconverter.h"
#include "ActorBasic.h"

// actor registration
std::map<std::string, CreateActorFn> actormap;




bool Theme::Load(const char* skinname) {
	// if previously theme loaded, then release it.
	ClearElements();

	// make metric path / skin path from skinname
	RString metricpath = RString(skinname) + ".skin.xml";
	RString skinpath = RString(skinname) + ".xml";		// can use xml, but better to use lua
	FileHelper::ConvertPathToSystem(metricpath);
	FileHelper::ConvertPathToSystem(skinpath);

	// load metric / skin from file
	// if file not exists, then attempt to convert lr2skin.
	if (!FileHelper::IsFile(metricpath)) {
		RString lr2skinpath = RString(skinname) + ".lr2skin";
		FileHelper::ConvertPathToSystem(lr2skinpath);
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

	// make options and set switch
	m_Skinmetric.GetDefaultOption(&m_Skinoption);
	m_Skinoption.SetEnvironmentFromOption();

	// make rendering tree from Skin element
	base_ = MakeActor(m_Skin.GetBaseElement());

	return true;
}

void Theme::ClearElements() {
	// before release rendertree and else, 
	// reset options from skinoption
	// (TODO)

	// release render trees
	// (will do automatically recursive)
	delete base_;

	// clear all
	base_ = 0;
	m_Skin.Clear();
	m_Skinmetric.Clear();
	m_Skinoption.Clear();

	// We won't release skin resource (It'll be soon reused, actually)
	// (But it should be done manually if you change or reload skin)
}

void Theme::Update() { 
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





// lua related
// TODO: LoadImage, LoadFont
// Make these array as singleton, as lua will act like running in single thread

std::map<std::string, Display::Texture*> lua_texmap;
std::map<std::string, Font*> lua_fontmap;

// input: table(path:string, key:string)
// output: bool
bool Lua_LoadImage(Lua* L) {
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
bool Lua_LoadFont(Lua* L) {
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
template <class Theme>
class LuaBinding {
public:
	void Register(Lua *L, int iMethods, int iMetatable) {
		luabridge::getGlobalNamespace(L)
			.addCFunction("LoadImage", Lua_LoadImage)
			.addCFunction("LoadFont", Lua_LoadFont);
	}
};


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