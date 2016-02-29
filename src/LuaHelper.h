#pragma once

#include "LuaManager.h"
#include <vector>

namespace LuaUtil {
	void RegisterBasic(lua_State* L);
}

namespace LuaHelper {
	using namespace std;

	template<class T>
	void Push(lua_State *L, const T &Object);

	template<class T>
	bool FromStack(lua_State *L, T &Object, int iOffset);

	template<class T>
	bool Pop(lua_State *L, T &val)
	{
		bool bRet = FromStack(L, val, -1);
		lua_pop(L, 1);
		return bRet;
	}

	int GetLuaStack(lua_State *L);
	bool RunExpression(Lua *L, const string &sExpression, const string &sName = "");
	bool RunScriptFile(Lua *L, const string &sFile, int ReturnValues = 0);
	bool RunScriptOnStack(Lua *L, string &Error, int Args, int ReturnValues, bool ReportError);
	bool RunScript(Lua *L, const string &Script, const string &Name, string &Error, int Args, int ReturnValues, bool ReportError);
}

//
// WARNING: this doesn't support deepcopy, be aware about this.
// COMMENT: if storing compiled lua binary chunk in binary(string) is possible, 
//          then is there any reason to deal with this structure ...?
//
class LuaFunc {
private:
	int m_funcid;
public:
	LuaFunc() : m_funcid(LUA_NOREF) {}
	~LuaFunc() { Remove(); }
	bool CompileScript(const std::string &script);
	void SetFromStack(lua_State *L);					// pops a stack
	void PushSelf(lua_State *L);
	void Remove();
};

//
class LuaFuncManager {
private:
	std::vector<LuaFunc*> funcpool_;
public:
	LuaFunc* CreateLuaFunc();
	void Clear();
};