#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
#include <mutex>
#include <assert.h>
using namespace tinyxml2;
//
// skin converter
//


namespace SkinConverter {
	// private function; for searching including file
	void SearchInclude(std::vector<std::string>& inc, XMLElement* node) {
		for (XMLElement* n = node;
			n;
			n = n->NextSiblingElement())
		{
			if (strcmp(n->Name(), "include") == 0) {
				inc.push_back(SkinUtil::GetAbsolutePath(n->Attribute("path")));
			}
			SearchInclude(inc, n->FirstChildElement());
		}
	}

	// depreciated; only for basic test, or 
	bool ConvertLR2SkinToXml(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".xml");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->Save(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".xml");
					// TODO: depart texturefont files?
					csvskin->Save(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return r;
	}

	bool ConvertLR2SkinToLua(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".lua");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->SaveToLua(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".lua");
					// TODO: depart texturefont files?
					csvskin->SaveToLua(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return false;
	}
}





// 
// LUA part
// - is redundant if you aren't going to test script.
//

#ifdef LUA
namespace LuaHelper
{
	using namespace std;
	template<> void Push<bool>(lua_State *L, const bool &Object) { lua_pushboolean(L, Object); }
	template<> void Push<float>(lua_State *L, const float &Object) { lua_pushnumber(L, Object); }
	template<> void Push<int>(lua_State *L, const int &Object) { lua_pushinteger(L, Object); }
	template<> void Push<unsigned int>(lua_State *L, const unsigned int &Object) { lua_pushnumber(L, double(Object)); }
	template<> void Push<string>(lua_State *L, const string &Object) { lua_pushlstring(L, Object.data(), Object.size()); }

	template<> bool FromStack<bool>(Lua *L, bool &Object, int iOffset) { Object = !!lua_toboolean(L, iOffset); return true; }
	template<> bool FromStack<float>(Lua *L, float &Object, int iOffset) { Object = (float)lua_tonumber(L, iOffset); return true; }
	template<> bool FromStack<int>(Lua *L, int &Object, int iOffset) { Object = lua_tointeger(L, iOffset); return true; }
	template<> bool FromStack<string>(Lua *L, string &Object, int iOffset)
	{
		size_t iLen;
		const char *pStr = lua_tolstring(L, iOffset, &iLen);
		if (pStr != NULL)
			Object.assign(pStr, iLen);
		else
			Object.clear();

		return pStr != NULL;
	}
}

namespace LuaHelper {
	using namespace luabridge;
	using namespace std;

	/*
	 * stack related
	 */
	bool IsBoolOnStack(lua_State *L) {
		return lua_isboolean(L, -1);
	}
	bool IsStringOnStack(lua_State *L) {
		return lua_isstring(L, -1);
	}
	bool IsNoneOnStack(lua_State *L) {
		return lua_isnil(L, -1);
	}

	/*
	 * This function is used to get whole Lua stack information
	 * All Lua stack information will be gathered into one stack.
	 */
	int GetLuaStack(lua_State *L)
	{
		string sErr;
		Pop(L, sErr);

		lua_Debug ar;

		for (int iLevel = 0; lua_getstack(L, iLevel, &ar); ++iLevel)
		{
			if (!lua_getinfo(L, "nSluf", &ar))
				break;
			// The function is now on the top of the stack.
			const char *file = ar.source[0] == '@' ? ar.source + 1 : ar.short_src;
			const char *name;
			vector<string> vArgs;

			if (!strcmp(ar.what, "C"))
			{
				for (int i = 1; i <= ar.nups && (name = lua_getupvalue(L, -1, i)) != NULL; ++i)
				{
					vArgs.push_back(ssprintf("%s = %s", name, lua_tostring(L, -1)));
					lua_pop(L, 1); // pop value
				}
			}
			else
			{
				for (int i = 1; (name = lua_getlocal(L, &ar, i)) != NULL; ++i)
				{
					vArgs.push_back(ssprintf("%s = %s", name, lua_tostring(L, -1)));
					lua_pop(L, 1); // pop value
				}
			}

			// If the first call is this function, omit it from the trace.
			if (iLevel == 0 && lua_iscfunction(L, -1) && lua_tocfunction(L, 1) == GetLuaStack)
			{
				lua_pop(L, 1); // pop function
				continue;
			}
			lua_pop(L, 1); // pop function

			sErr += ssprintf("\n%s:", file);
			if (ar.currentline != -1)
				sErr += ssprintf("%i:", ar.currentline);

			if (ar.name && ar.name[0])
				sErr += ssprintf(" %s", ar.name);
			else if (!strcmp(ar.what, "main") || !strcmp(ar.what, "tail") || !strcmp(ar.what, "C"))
				sErr += ssprintf(" %s", ar.what);
			else
				sErr += ssprintf(" unknown");
			// join string with ","
			sErr += "(";
			for (int i = 0; i < vArgs.size(); i++) {
				sErr += vArgs[i] + ",";
			}
			if (vArgs.size()) sErr.pop_back();
			sErr += ")";
		}

		Push(L, sErr);
		return 1;
	}

	bool RunScriptOnStack(Lua *L, string &Error, int Args, int ReturnValues, bool ReportError)
	{
		lua_pushcfunction(L, GetLuaStack);

		// move the error function above the function and params
		int ErrFunc = lua_gettop(L) - Args - 1;
		lua_insert(L, ErrFunc);

		// evaluate
		int ret = lua_pcall(L, Args, ReturnValues, ErrFunc);
		if (ret)
		{
			if (ReportError)
			{
				string lerror;
				Pop(L, lerror);
				Error += lerror;
				printf("Lua Error: %s\n", lerror.c_str());
				//ScriptErrorMessage(Error);
			}
			else
			{
				Pop(L, Error);
			}
			lua_remove(L, ErrFunc);
			for (int i = 0; i < ReturnValues; ++i)
				lua_pushnil(L);
			return false;
		}

		lua_remove(L, ErrFunc);
		return true;
	}

	/** private */
	namespace {
#define READBUFSIZE 1024
		bool GetFileContents(const char* luapath, string& script) {
			char buf[READBUFSIZE + 1];
			FILE *f = SkinUtil::OpenFile(luapath, "r");
			if (!f)
				return false;
			int s;
			while (s = fread(buf, 1, READBUFSIZE, f)) {
				buf[s] = 0;
				script.append(buf);
			}
			fclose(f);
			return true;
		}
	}

	bool RunScriptFile(Lua *L, const string &sFile, int ReturnValues)
	{
		string sScript;
		if (!GetFileContents(sFile.c_str(), sScript))
			return false;

		string sError;
		if (!RunScript(L, sScript, "@" + sFile, sError, 0, ReturnValues, true))
		{
			printf("Lua runtime error: %s", sError.c_str());
			//LuaHelpers::ReportScriptError(sError);
			return false;
		}

		return true;
	}

	bool RunExpression(Lua *L, const string &sExpression, const string &sName)
	{
		string sError = ssprintf("Lua runtime error parsing \"%s\": ", sName.size() ? sName.c_str() : sExpression.c_str());
		if (!RunScript(L, "return " + sExpression, sName.empty() ? string("in") : sName, sError, 0, 1, true))
		{
			return false;
		}
		return true;
	}

	bool LoadScript(Lua *L, const string &sScript, const string &sName, string &sError)
	{
		// load string into lua stack
		int ret = luaL_loadbuffer(L, sScript.data(), sScript.size(), sName.c_str());
		if (ret)
		{
			Pop(L, sError);
			return false;
		}
		return true;
	}

	bool RunScript(Lua *L, const string &Script, const string &Name, string &Error, int Args, int ReturnValues, bool ReportError)
	{
		string lerror;
		if (!LoadScript(L, Script, Name, lerror))
		{
			Error += lerror;
			if (ReportError)
			{
				//ScriptErrorMessage(Error);
				printf("Lua Error: %s\n", lerror.c_str());
			}
			lua_pop(L, Args);
			for (int i = 0; i < ReturnValues; ++i)
				lua_pushnil(L);
			return false;
		}

		// move the function above the params
		lua_insert(L, lua_gettop(L) - Args);

		return RunScriptOnStack(L, Error, Args, ReturnValues, ReportError);
	}
}

namespace LuaRunner {
	//
	// lua core vars
	//
	// basic funcs:
	// - CND
	// - DST
	// Defs
	// - Def.sprite
	// - Def.text
	// - ...
	// set import path
	// http://stackoverflow.com/questions/22492741/lua-require-function-does-not-find-my-required-file-on-ios
	//
	using namespace luabridge;

	// originally: calculate condition
	bool Luna_CND(const std::string& cnd) {
		// do nothing ...
		// but we should inspect all conditional clause,
		// so always return true.
		printf("received CND: %s\n", cnd.c_str());
		return true;
	}

	void Luna_warn(const std::string& msg) {
		printf("Lua Script warning: %s\n", msg.c_str());
	}

	// register basic functions / classes
	namespace Util {
		bool IsRegisteredClass(const std::string& cls) {
			// in debug, always return true
			return true;
		}
		std::string GetDir(const std::string& path) {
			//printf("Dir %s\n", path.c_str());
			return "test";
		}
		std::string ResolvePath(const std::string& relpath) {
			// temporarily function; it should convert relpath to absolute path, in fact.
			return relpath;
		}
		std::string GetFileType(const std::string& relpath) {
			// temporarily function; get file type
			int p = relpath.find_last_of('.');
			if (p != std::string::npos) {
				std::string ext = relpath.substr(p+1);
				if (ext == "lua")
					return "Lua";
				else if (ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "png")
					return "Image";
				else if (ext == "mp4" || ext == "avi" || ext == "mpeg" || ext == "mpg")
					return "Movie";
			}
			return "Unknown";
		}
	};
	void RegisterBasic(lua_State* L) {
		Namespace& ln = getGlobalNamespace(L);
		ln.addFunction("CND", Luna_CND);
		ln.addFunction("warn", Luna_warn);

		// refer: Utilizing library ... http://www.lua.org/pil/28.1.html
		ln.beginNamespace("Util")
			.addFunction("IsRegisteredClass", Util::IsRegisteredClass)
			.addFunction("GetDir", Util::GetDir)
			.addFunction("ResolvePath", Util::ResolvePath)
			.addFunction("GetDir", Util::GetDir)
			.endNamespace();
	}

	lua_State *L;
	bool luainitalized = false;
	void InitLua() {
		assert(luainitalized == false);

		L = lua_open();
		lua_pushcfunction(L, luaopen_base); lua_call(L, 0, 0);
		lua_pushcfunction(L, luaopen_math); lua_call(L, 0, 0);
		lua_pushcfunction(L, luaopen_string); lua_call(L, 0, 0);
		lua_pushcfunction(L, luaopen_table); lua_call(L, 0, 0);
		lua_pushcfunction(L, luaopen_debug); lua_call(L, 0, 0);
		lua_pushcfunction(L, luaopen_package); lua_call(L, 0, 0);

		// Register default inner global function
		RegisterBasic(L);

		// Register default library path
		// (TODO)

		// Load default library into stack (CreateMethodsTable?)
		// COMMENT: we can set metatable on raw c pointer object, refer:
		// http://stackoverflow.com/questions/14618002/how-to-return-c-object-to-lua-5-2
		// http://lua-users.org/lists/lua-l/2006-08/msg00245.html
		LuaHelper::RunScriptFile(L, "../system/script/common.lua");

		luainitalized = true;
	}

	void CloseLua() {
		assert(luainitalized);
		lua_close(L);
		luainitalized = false;
	}

	lua_State* Get() {
		// don't use thread pool now, so clean stack.
		assert(lua_gettop(L) == 0);
		lua_State *pRet = lua_newthread(L);
		return pRet;
	}

	void Release(lua_State* p) {
		// must process all stacks before release it
		assert(lua_gettop(p) == 0);
		p = NULL;
	}
}
#endif

namespace SkinTest {
	// ////////////////////////////////////////////////////////
	// lua testing part
	// ////////////////////////////////////////////////////////
#ifdef LUA
	//
	// parsing related vars
	//
	std::vector<std::string> parselist;
	std::mutex m_lualock;
	Lua* l = 0;
	bool TestLuaFile(const char* luapath) {
		return LuaHelper::RunScriptFile(l, luapath, 1);
	}

	void InitLua() {
		LuaRunner::InitLua();
	}

	void CloseLua() {
		LuaRunner::CloseLua();
	}

	// check: LoadFromNode()
	// LoadXNodeFromLuaShowErrors
	// XNodeFromTable
	// -> So, in fact, it creates xml table for itself!
	// refer https://github.com/stepmania/stepmania/blob/536be1f64c215cdf53eadbf29926a2c5d82a4518/src/XmlFileUtil.cpp
	// -> get table as return argument.
	// * If this entry is a table, add it recursively.
	// * Otherwise, add an attribute.

	int printindent = 0;
	inline void PrintIndent() {
		printf("-");
		for (int i = 0; i < printindent;i++)
			printf("  ");
	}
	void PrintTable(lua_State* l) {
		// parses table and tell it's state.
		lua_pushnil(l);
		while (lua_next(l, -2)) {
			// copy key, so lua_tostring won't modify original key
			lua_pushvalue(l, -2);
			std::string key = lua_tostring(l, -1);
			lua_pop(l, 1);

			if (lua_isstring(l, -1)) {
				PrintIndent();
				printf("%s=%s (str)\n", key.c_str(), lua_tostring(l, -1));
			}
			else if (lua_isnumber(l, -1)) {
				PrintIndent();
				printf("%s=%s (int)\n", key.c_str(), lua_tostring(l, -1));
			}
			else if (lua_istable(l, -1)) {
				printf("%s (table)\n", key.c_str());
				//printf("table\n");
				printindent++;
				PrintTable(l);
				printindent--;
			}
			else {
				PrintIndent();
				printf("%s (unknown)\n", key.c_str());
			}
			lua_pop(l, 1);		// pop current attribute
		}
	}

	// private end, public start

	/*
	** DO NOT run this function in multi thread
	** although this func will be automatically locked...
	*/
	bool TestLuaSkin(const char* luapath) {
		m_lualock.lock();
		parselist.push_back(luapath);
		bool r = true;
		if (!l)
			l = LuaRunner::Get();
		printf("stack before run: %d\n", lua_gettop(l));
		while (parselist.size()) {
			std::string path = parselist[0];
			parselist.erase(parselist.begin());
			r &= TestLuaFile(path.c_str());
		}
		printf("stack after run: %d\n", lua_gettop(l));
		// we need to extract table by ourselves
		{
			int t = lua_type(l, -1);
			if (t == LUA_TTABLE) {
				printf("okay its table\n");
				printindent = 0;
				PrintTable(l);
				lua_pop(l, 1);		// pop table
			}
			else if (t == LUA_TNUMBER) {
				int ret;
				LuaHelper::Pop(l, ret);
				printf("okay its int, value is %d\n", ret);
			}
			else {
				printf("I can\'t figure out what it this ... nil?\n");
				lua_pop(l, 1);		// pop anyway
			}
		}
		printf("stack finally: %d\n", lua_gettop(l));
		m_lualock.unlock();
		return r;
	}
#endif
}