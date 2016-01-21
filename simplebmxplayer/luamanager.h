#pragma once

#include <string>
#include <vector>
using namespace std;

struct lua_State;
typedef lua_State Lua;
typedef void(*RegisterWithLuaFn)(lua_State*);

extern "C"
{
#include "Lua/lua.h"
#include "Lua/lualib.h"
#include "Lua/lauxlib.h"
}

#include "global.h"

/*
 * Manages & stores function/variable in Lua VM
 * CLAIM: part of sourcecode was refered from Stepmania, all rights reserved.
 */
class LuaManager {
public:
	// Every Actor should register its class at program initialization.
	static void Register(RegisterWithLuaFn pfn);

	LuaManager();
	~LuaManager();

	Lua *Get();
	void Release(Lua *&p);

	/* Explicitly lock and unlock Lua access. This is done automatically by
	* Get() and Release(). */
	void YieldLua();
	void UnyieldLua();
	
	// Register all types which is registered in (void)Register
	void RegisterTypes();

	void SetGlobal(const RString &sName, int val);
	void SetGlobal(const RString &sName, const RString &val);
	void UnsetGlobal(const RString &sName);
private:
	lua_State *m_pLuaMain;
	// Swallow up warnings. If they must be used, define them.
	LuaManager& operator=(const LuaManager& rhs);
	LuaManager(const LuaManager& rhs);
};


/** @brief Utilities for working with Lua. */
namespace LuaHelpers
{
	/* Load the given script with the given name. On success, the resulting
	* chunk will be on the stack. On error, the error is stored in sError
	* and the stack is unchanged. */
	bool LoadScript(Lua *L, const RString &sScript, const RString &sName, RString &sError);

	// Just the broadcast message part, for things that need to do the rest differently.
	void ScriptErrorMessage(RString const& Error);
	// For convenience when replacing uses of LOG->Warn.
	void ReportScriptErrorFmt(const char *fmt, ...);

	/* Run the function with arguments at the top of the stack, with the given
	* number of arguments. The specified number of return values are left on
	* the Lua stack. On error, nils are left on the stack, sError is set and
	* false is returned.
	* If ReportError is true, Error should contain the string to prepend
	* when reporting.  The error is reported through LOG->Warn and
	* SCREENMAN->SystemMessage.
	*/
	bool RunScriptOnStack(Lua *L, RString &Error, int Args = 0, int ReturnValues = 0, bool ReportError = false);

	/* LoadScript the given script, and RunScriptOnStack it.
	* iArgs arguments are at the top of the stack. */
	bool RunScript(Lua *L, const RString &Script, const RString &Name, RString &Error, int Args = 0, int ReturnValues = 0, bool ReportError = false);

	/* Run the given expression, returning a single value, and leave the return
	* value on the stack.  On error, push nil. */
	bool RunExpression(Lua *L, const RString &sExpression, const RString &sName = "");

	bool RunScriptFile(const RString &sFile);

	/* Create a Lua array (a table with indices starting at 1) of the given vector,
	* and push it on the stack. */
	void CreateTableFromArrayB(Lua *L, const vector<bool> &aIn);

	/* Recursively copy elements from the table at stack element -2 into the
	* table at stack -1. Pop both elements from the stack. */
	void DeepCopy(lua_State *L);

	// Read the table at the top of the stack back into a vector.
	void ReadArrayFromTableB(Lua *L, vector<bool> &aOut);

	/* Pops the last iArgs arguments from the stack, and return a function that
	* returns those values. */
	void PushValueFunc(lua_State *L, int iArgs);

	template<class T>
	void Push(lua_State *L, const T &Object);

	template<class T>
	bool FromStack(lua_State *L, T &Object, int iOffset);

	template<class T>
	bool Pop(lua_State *L, T &val)
	{
		bool bRet = LuaHelpers::FromStack(L, val, -1);
		lua_pop(L, 1);
		return bRet;
	}

	template<class T>
	void ReadArrayFromTable(vector<T> &aOut, lua_State *L)
	{
		luaL_checktype(L, -1, LUA_TTABLE);

		int i = 0;
		while (lua_rawgeti(L, -1, ++i), !lua_isnil(L, -1))
		{
			T value = T();
			LuaHelpers::Pop(L, value);
			aOut.push_back(value);
		}
		lua_pop(L, 1); // pop nil
	}
	template<class T>
	void CreateTableFromArray(const vector<T> &aIn, lua_State *L)
	{
		lua_newtable(L);
		for (unsigned i = 0; i < aIn.size(); ++i)
		{
			LuaHelpers::Push(L, aIn[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	int TypeError(Lua *L, int narg, const char *tname);
	inline int AbsIndex(Lua *L, int i) { if (i > 0 || i <= LUA_REGISTRYINDEX) return i; return lua_gettop(L) + i + 1; }
}

/*
 * Lua Function Register macro
 */
struct RegisterLuaFunction { RegisterLuaFunction(RegisterWithLuaFn pfn) { LuaManager::Register(pfn); } };
#define REGISTER_WITH_LUA_FUNCTION( Fn ) \
	static RegisterLuaFunction register##Fn( Fn );

#define LuaFunction( func, expr ) \
int LuaFunc_##func( lua_State *L ); \
int LuaFunc_##func( lua_State *L ) { \
	LuaHelpers::Push( L, expr ); return 1; \
} \
void LuaFunc_Register_##func( lua_State *L ); \
void LuaFunc_Register_##func( lua_State *L ) { lua_register( L, #func, LuaFunc_##func ); } \
REGISTER_WITH_LUA_FUNCTION( LuaFunc_Register_##func );

#define LUAFUNC_REGISTER_COMMON(func_name) \
void LuaFunc_Register_##func_name(lua_State* L); \
void LuaFunc_Register_##func_name(lua_State* L) { lua_register(L, #func_name, LuaFunc_##func_name); } \
REGISTER_WITH_LUA_FUNCTION(LuaFunc_Register_##func_name);

/*
 * Accessible in any part of the program
 */

extern LuaManager* LUA;