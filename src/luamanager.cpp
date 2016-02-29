#include "LuaManager.h"
#include <assert.h>

// not initalized basically
LuaManager* LUA = 0;


// luamanager part

namespace {
	// privates
}

LuaManager::LuaManager() {
	L = lua_open();
	lua_pushcfunction(L, luaopen_base); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_math); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_string); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_table); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_debug); lua_call(L, 0, 0);
	lua_pushcfunction(L, luaopen_package); lua_call(L, 0, 0);
};

LuaManager::~LuaManager() {
	lua_close(L);
}

Lua* LuaManager::Get() {
	// we won't care about multi-threading, but we should care about mutex of multi thread
	m_mutex.lock();
	return lua_newthread(L);
}

void LuaManager::Release(Lua* l) {
	// must process all lua stack before release it
	assert(lua_gettop(l) == 0);
	m_mutex.unlock();
	// coroutine thread don't do lua_close()
	//lua_close(l);
}

// luamanager end