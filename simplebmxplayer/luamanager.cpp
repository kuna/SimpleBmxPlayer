#include "luamanager.h"
#include "util.h"
#include "logger.h"
#include <map>
#include <mutex>

/** global var */
LuaManager* LUA = 0;

/*
 * This structure stores Each Lua Objects,
 * and Lock one if one is using. after use, unlock it.
 */
struct Impl
{
	vector<lua_State *> g_FreeStateList;
	map<lua_State *, bool> g_ActiveStates;
	mutex g_pLock;
};
static Impl *pImpl = NULL;

/** @brief Utilities for working with Lua. */
namespace LuaHelpers
{
	template<> void Push<bool>(lua_State *L, const bool &Object);
	template<> void Push<float>(lua_State *L, const float &Object);
	template<> void Push<int>(lua_State *L, const int &Object);
	template<> void Push<RString>(lua_State *L, const RString &Object);

	template<> bool FromStack<bool>(Lua *L, bool &Object, int iOffset);
	template<> bool FromStack<float>(Lua *L, float &Object, int iOffset);
	template<> bool FromStack<int>(Lua *L, int &Object, int iOffset);
	template<> bool FromStack<RString>(Lua *L, RString &Object, int iOffset);

	bool InReportScriptError = false;
}

void LuaManager::SetGlobal(const RString &sName, int val)
{
	Lua *L = Get();
	LuaHelpers::Push(L, val);
	lua_setglobal(L, sName);
	Release(L);
}

void LuaManager::SetGlobal(const RString &sName, const RString &val)
{
	Lua *L = Get();
	LuaHelpers::Push(L, val);
	lua_setglobal(L, sName);
	Release(L);
}

void LuaManager::UnsetGlobal(const RString &sName)
{
	Lua *L = Get();
	lua_pushnil(L);
	lua_setglobal(L, sName);
	Release(L);
}

/** @brief Utilities for working with Lua. */
namespace LuaHelpers
{
	template<> void Push<bool>(lua_State *L, const bool &Object) { lua_pushboolean(L, Object); }
	template<> void Push<float>(lua_State *L, const float &Object) { lua_pushnumber(L, Object); }
	template<> void Push<int>(lua_State *L, const int &Object) { lua_pushinteger(L, Object); }
	template<> void Push<unsigned int>(lua_State *L, const unsigned int &Object) { lua_pushnumber(L, double(Object)); }
	template<> void Push<RString>(lua_State *L, const RString &Object) { lua_pushlstring(L, Object.data(), Object.size()); }

	template<> bool FromStack<bool>(Lua *L, bool &Object, int iOffset) { Object = !!lua_toboolean(L, iOffset); return true; }
	template<> bool FromStack<float>(Lua *L, float &Object, int iOffset) { Object = (float)lua_tonumber(L, iOffset); return true; }
	template<> bool FromStack<int>(Lua *L, int &Object, int iOffset) { Object = lua_tointeger(L, iOffset); return true; }
	template<> bool FromStack<RString>(Lua *L, RString &Object, int iOffset)
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

void LuaHelpers::CreateTableFromArrayB(Lua *L, const vector<bool> &aIn)
{
	lua_newtable(L);
	for (unsigned i = 0; i < aIn.size(); ++i)
	{
		lua_pushboolean(L, aIn[i]);
		lua_rawseti(L, -2, i + 1);
	}
}

void LuaHelpers::ReadArrayFromTableB(Lua *L, vector<bool> &aOut)
{
	luaL_checktype(L, -1, LUA_TTABLE);

	for (unsigned i = 0; i < aOut.size(); ++i)
	{
		lua_rawgeti(L, -1, i + 1);
		bool bOn = !!lua_toboolean(L, -1);
		aOut[i] = bOn;
		lua_pop(L, 1);
	}
}

/*
 * This function is used to get whole Lua stack information
 * All Lua stack information will be gathered into one stack.
 */
static int GetLuaStack(lua_State *L)
{
	RString sErr;
	LuaHelpers::Pop(L, sErr);

	lua_Debug ar;

	for (int iLevel = 0; lua_getstack(L, iLevel, &ar); ++iLevel)
	{
		if (!lua_getinfo(L, "nSluf", &ar))
			break;
		// The function is now on the top of the stack.
		const char *file = ar.source[0] == '@' ? ar.source + 1 : ar.short_src;
		const char *name;
		vector<RString> vArgs;

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
		sErr += ssprintf("(%s)", join(",", vArgs).c_str());
	}

	LuaHelpers::Push(L, sErr);
	return 1;
}


static int LuaPanic(lua_State *L)
{
	GetLuaStack(L);

	RString sErr;
	LuaHelpers::Pop(L, sErr);

	LOG->Critical(sErr);
	// TODO: we have to process it with a dialog
	ASSERT(0);
	//RageException::Throw("[Lua panic] %s", sErr.c_str());

	return 0;
}

// Actor registration
static vector<RegisterWithLuaFn>	*g_vRegisterActorTypes = NULL;

void LuaManager::Register(RegisterWithLuaFn pfn)
{
	if (g_vRegisterActorTypes == NULL)
		g_vRegisterActorTypes = new vector<RegisterWithLuaFn>;

	g_vRegisterActorTypes->push_back(pfn);
}


LuaManager::LuaManager()
{
	pImpl = new Impl;
	LUA = this; // so that LUA is available when we call the Register functions

	lua_State *L = lua_open();
	ASSERT(L != NULL);

	lua_atpanic(L, LuaPanic);
	m_pLuaMain = L;

	lua_pushcfunction(L, luaopen_base); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_math); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_string); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_table); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_debug); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_package); lua_call(L, 0, 0); // this one seems safe -shake
	// these two can be dangerous. don't use them
	// (unless you know what you are doing). -aj
#if 0
	lua_pushcfunction(L, luaopen_io); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_os); lua_call(L, 0, 0);
#endif

	// Store the thread pool in a table on the stack, in the main thread.
#define THREAD_POOL 1
	lua_newtable(L);

	RegisterTypes();
}

LuaManager::~LuaManager()
{
	lua_close(m_pLuaMain);
	SAFE_DELETE(pImpl);
}

void LuaManager::RegisterTypes()
{
	Lua *L = Get();

	if (g_vRegisterActorTypes)
	{
		for (unsigned i = 0; i<g_vRegisterActorTypes->size(); i++)
		{
			RegisterWithLuaFn fn = (*g_vRegisterActorTypes)[i];
			fn(L);
		}
	}

	Release(L);
}

/*
 * Need to give THREAD_POOL as Lua argument
 */
Lua *LuaManager::Get()
{
	bool bLocked = false;
	// TODO: need safe way to lock Lua Thread
	// (in this case, same thread cannot get multiple Lua object)
	//if (!pImpl->g_pLock.IsLockedByThisThread())
	//{
		pImpl->g_pLock.lock();
		bLocked = true;
	//}

	ASSERT(lua_gettop(m_pLuaMain) == 1);

	lua_State *pRet;
	if (pImpl->g_FreeStateList.empty())
	{
		pRet = lua_newthread(m_pLuaMain);

		// Store the new thread in THREAD_POOL, so it isn't collected.
		int iLast = lua_objlen(m_pLuaMain, THREAD_POOL);
		lua_rawseti(m_pLuaMain, THREAD_POOL, iLast + 1);
	}
	else
	{
		pRet = pImpl->g_FreeStateList.back();
		pImpl->g_FreeStateList.pop_back();
	}

	pImpl->g_ActiveStates[pRet] = bLocked;
	return pRet;
}

void LuaManager::Release(Lua *&p)
{
	pImpl->g_FreeStateList.push_back(p);

	ASSERT(lua_gettop(p) == 0);
	ASSERT(pImpl->g_ActiveStates.find(p) != pImpl->g_ActiveStates.end());
	bool bDoUnlock = pImpl->g_ActiveStates[p];
	pImpl->g_ActiveStates.erase(p);

	if (bDoUnlock)
		pImpl->g_pLock.unlock();
	p = NULL;
}

/*
 * Helper part
 * - It just runs a script in a local 
 */
bool LuaHelpers::RunScriptFile(const RString &sFile)
{
	RString sScript;
	if (!GetFileContents(sFile, sScript))
		return false;

	Lua *L = LUA->Get();

	RString sError;
	if (!LuaHelpers::RunScript(L, sScript, "@" + sFile, sError, 0))
	{
		printf(ssprintf("Lua runtime error: %s", sError.c_str()));
		//LuaHelpers::ReportScriptError(sError);
		return false;
	}
	LUA->Release(L);

	return true;
}


bool LuaHelpers::LoadScript(Lua *L, const RString &sScript, const RString &sName, RString &sError)
{
	// load string
	int ret = luaL_loadbuffer(L, sScript.data(), sScript.size(), sName);
	if (ret)
	{
		LuaHelpers::Pop(L, sError);
		return false;
	}

	return true;
}

void LuaHelpers::ScriptErrorMessage(RString const& Error)
{
	LOG->Critical("ScriptError: %d", Error);
}

// For convenience when replacing uses of LOG->Warn.
void LuaHelpers::ReportScriptErrorFmt(const char *fmt, ...)
{
	va_list	va;
	va_start(va, fmt);
	RString Buff = vssprintf(fmt, va);
	va_end(va);
	ScriptErrorMessage(Buff);
}

bool LuaHelpers::RunScriptOnStack(Lua *L, RString &Error, int Args, int ReturnValues, bool ReportError)
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
			RString lerror;
			LuaHelpers::Pop(L, lerror);
			Error += lerror;
			ScriptErrorMessage(Error);
		}
		else
		{
			LuaHelpers::Pop(L, Error);
		}
		lua_remove(L, ErrFunc);
		for (int i = 0; i < ReturnValues; ++i)
			lua_pushnil(L);
		return false;
	}

	lua_remove(L, ErrFunc);
	return true;
}

bool LuaHelpers::RunScript(Lua *L, const RString &Script, const RString &Name, RString &Error, int Args, int ReturnValues, bool ReportError)
{
	RString lerror;
	if (!LoadScript(L, Script, Name, lerror))
	{
		Error += lerror;
		if (ReportError)
		{
			ScriptErrorMessage(Error);
		}
		lua_pop(L, Args);
		for (int i = 0; i < ReturnValues; ++i)
			lua_pushnil(L);
		return false;
	}

	// move the function above the params
	lua_insert(L, lua_gettop(L) - Args);

	return LuaHelpers::RunScriptOnStack(L, Error, Args, ReturnValues, ReportError);
}

bool LuaHelpers::RunExpression(Lua *L, const RString &sExpression, const RString &sName)
{
	RString sError = ssprintf("Lua runtime error parsing \"%s\": ", sName.size() ? sName.c_str() : sExpression.c_str());
	if (!LuaHelpers::RunScript(L, "return " + sExpression, sName.empty() ? RString("in") : sName, sError, 0, 1, true))
	{
		return false;
	}
	return true;
}

namespace
{
	int lua_pushvalues(lua_State *L)
	{
		int iArgs = lua_tointeger(L, lua_upvalueindex(1));
		for (int i = 0; i < iArgs; ++i)
			lua_pushvalue(L, lua_upvalueindex(i + 2));
		return iArgs;
	}
}

void LuaHelpers::PushValueFunc(lua_State *L, int iArgs)
{
	int iTop = lua_gettop(L) - iArgs + 1;
	lua_pushinteger(L, iArgs);
	lua_insert(L, iTop);
	lua_pushcclosure(L, lua_pushvalues, iArgs + 1);
}