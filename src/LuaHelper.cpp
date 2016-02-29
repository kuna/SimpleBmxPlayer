#include "LuaHelper.h"
#include "skinutil.h"
#include <string>
#include <sstream>
#include <vector>


/*
 * LuaUtil:
 * general utilities which must be executed before lua code proceed
 */
namespace LuaUtil {
	using namespace luabridge;

	void luawarn(const std::string& msg) {
		printf("Lua Script warning: %s\n", msg.c_str());
	}

	void luatrace(const std::string& msg) {
		printf("Lua trace: %s\n", msg.c_str());
	}

	// originally: calculate condition
	bool CND(const std::string& cnd) {
		// do nothing now ...
		// but we should inspect all conditional clause,
		// so always return true.
		printf("received CND: %s\n", cnd.c_str());
		return true;
	}

	int DST(lua_State* L) {
		std::ostringstream ss;
		ss << "function(s) ";
		// input 1 argument; must be table
		if (!lua_istable(L, -1)) {
			luawarn("received wrong lua DST object.");
		}
		else {
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				if (lua_isnumber(L, -2)) {
					luawarn("key of DST argument cannot be number");
					continue;
				}
				if (lua_isnumber(L, -1)) {
					ss << ssprintf("s:%s(%d); ", lua_tostring(L, -2), lua_tointeger(L, -1));
				}
				else {
					ss << ssprintf("s:%s(\"%s\"); ", lua_tostring(L, -2), lua_tostring(L, -1));
				}
				lua_pop(L, 1);
			}
		}
		// pop current argument(DST table) ...
		lua_pop(L, 1);
		ss << "end";
		lua_pushstring(L, ss.str().c_str());

		// return 1 argument; string formatted lua function
		return 1;
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
			// temporarily function ... csv file is automatically changed into lua to find path
			int p = relpath.find_last_of('.');
			if (p != std::string::npos) {
				std::string ext = relpath.substr(p + 1);
				if (ext == "csv")
					return SkinUtil::GetAbsolutePath(relpath.substr(0, p) + ".lua");
			}
			return SkinUtil::GetAbsolutePath(relpath);
		}
		std::string GetFileType(const std::string& relpath) {
			// temporarily function; get file type
			int p = relpath.find_last_of('.');
			if (p != std::string::npos) {
				std::string ext = relpath.substr(p + 1);
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
		ln.addFunction("CND", CND);
		ln.addFunction("warn", luawarn);
		ln.addFunction("trace", luatrace);
		ln.addCFunction("DST", DST);

		// refer: Utilizing library ... http://www.lua.org/pil/28.1.html
		ln.beginNamespace("Util")
			.addFunction("IsRegisteredClass", Util::IsRegisteredClass)
			.addFunction("GetDir", Util::GetDir)
			.addFunction("GetFileType", Util::GetFileType)
			.addFunction("ResolvePath", Util::ResolvePath)
			.endNamespace();

		// load basic library
		LuaHelper::RunScriptFile(L, "../system/script/common.lua");
	}
}

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
			printf("Lua runtime error: %s\n", sError.c_str());
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




////





void LuaFunc::SetFromStack(lua_State* L) {
	assert(lua_isfunction(L, -1));
	if (m_funcid != LUA_NOREF)
		Remove();
	m_funcid = luaL_ref(L, LUA_REGISTRYINDEX);
}

void LuaFunc::PushSelf(lua_State* L) {
	assert(m_funcid != LUA_NOREF);
	lua_rawgeti(L, LUA_REGISTRYINDEX, m_funcid);
}

bool LuaFunc::CompileScript(const std::string& script) {
	Lua *L = LUA->Get();
	bool r = LuaHelper::RunExpression(L, script);
	SetFromStack(L);
	LUA->Release(L);
	return r;
}

void LuaFunc::Remove() {
	if (m_funcid != LUA_NOREF) {
		// avoid deadlock - is more important, maybe?
		// but it can be a little unstable
		Lua *L = LUA->GetPtr();
		luaL_unref(L, LUA_REGISTRYINDEX, m_funcid);
		//LUA->Release(L);
	}
}


LuaFunc* LuaFuncManager::CreateLuaFunc() {
	LuaFunc* fn = new LuaFunc();
	funcpool_.push_back(fn);
	return fn;
}

void LuaFuncManager::Clear() {
	for (int i = 0; i < funcpool_.size(); i++) {
		delete funcpool_[i];
	}
	funcpool_.clear();
}