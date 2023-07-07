#include <uescript.h>
#include "sdk.h"

#include "uescript/uescript_sdk.h"
#include "unreal/unreal_sdk.h"
#include "windows/windows_sdk.h"
#include "signature/signature_sdk.h"

static const std::initializer_list<std::unique_ptr<LuaSDK>> SDKRegistry = {
    std::make_unique<UEScriptSDK>(),
    std::make_unique<UnrealSDK>(),
    std::make_unique<WindowsSDK>(),
    std::make_unique<SignatureSDK>(),
};

void LuaSDK::Init(lua_State* L)
{
    LUAJIT_VERSION_SYM();
    lua_atpanic(L, Panic);

    luaL_openlibs(L);
    luaL_dostring(L, "jit.opt.start(3)");
    // Parameters taken from OpenResty's luajit2
    luaL_dostring(L, R"(jit.opt.start("maxtrace=8000", "maxrecord=16000", "minstitch=3", "maxmcode=40960"))");

    lua_pushcfunction(L, Print);
    lua_setglobal(L, "print");

    for (const std::unique_ptr<LuaSDK>& sdk : SDKRegistry)
        sdk->InitInternal(L);
}

void LuaSDK::PrintCallstack(lua_State* L)
{
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_call(L, 0, 1);
    std::cout << "CALLSTACK: " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 2);
}

/* Taken from LuaJIT */
int LuaSDK::Print(lua_State* L)
{
    const int n = lua_gettop(L); /* number of arguments */
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++)
    {
        lua_pushvalue(L, -1); /* function to be called */
        lua_pushvalue(L, i); /* value to print */
        if (const int status = lua_pcall(L, 1, 1, 0); status != LUA_OK)
            return luaL_error(L, LUA_QL("tostring") " error");
        size_t sz;
        const char* s = lua_tolstring(L, -1, &sz); /* get result */
        if (s == nullptr)
            return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
        if (i > 1)
            (void)putc('\t', stdout);
        (void)fwrite(s, 1, sz, stdout);
        lua_pop(L, 1); /* pop result */
    }
    (void)putc('\n', stdout);
    (void)fflush(stdout);
    return 0;
}

int LuaSDK::Panic(lua_State* L)
{
    const char* message = lua_tostring(L, -1);
    std::cerr << "LUA PANIC: " << message << std::endl;
#ifdef _DEBUG
    __debugbreak();
#endif
    return 0;
}
