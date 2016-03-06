/*
 * @description
 * initalize / manages lua object
 */

#ifndef LUAMANAGER
#define LUAMANAGER 1

#include <mutex>

extern "C" {
#include "Lua/lua.h"
#include "Lua/lualib.h"
#include "Lua/lauxlib.h"
}
#include "LuaBridge.h"

typedef lua_State Lua;

class LuaManager {
private:
	Lua* L;
	std::mutex m_mutex;
public:
	LuaManager();
	~LuaManager();
	Lua* GetPtr() { return L; };
	Lua* Get();
	void Release(Lua*);
};

// global object
extern LuaManager* LUA;




// All of these binding will be processed in game's initalization process
template <class T>
class LuaBinding {
public:
	static void Register(Lua *L, int iMethods, int iMetatable);
};



// some macros for easy lua-programming
#define RETURN_TRUE_LUA lua_pushboolean(L, 1); return 1;
#define RETURN_FALSE_LUA lua_pushboolean(L, 0); return 1;
#define GETKEY_LUA(item) lua_pushstring(L, item); lua_gettable(L, -2);

#endif