#include <uescript.h>
#include "uescript_sdk.h"

#include <lua/lua_engine.h>
#include <utils/allocations.h>

void UEScriptSDK::InitInternal(lua_State* L)
{
    lua_newtable(L);
    {
        lua_pushcfunction(L, UEScriptSDK::LoadString);
        lua_setfield(L, -2, "LoadString");

        lua_pushcfunction(L, UEScriptSDK::RunString);
        lua_setfield(L, -2, "RunString");

        lua_pushcfunction(L, UEScriptSDK::DirIter);
        lua_setfield(L, -2, "DirIter");

        lua_pushstring(L, LuaEngine::GetHomeDirectory().string().c_str());
        lua_setfield(L, -2, "Home");

        DefineIntPtrOp<int8_t>(L);
        DefineIntPtrOp<int16_t>(L);
        DefineIntPtrOp<int32_t>(L);
        DefineIntPtrOp<int64_t>(L);

        lua_pushcfunction(L, PtrRead<float>);
        lua_setfield(L, -2, "ReadFloat32");
        lua_pushcfunction(L, PtrRead<double>);
        lua_setfield(L, -2, "ReadFloat64");

        lua_pushcfunction(L, PtrWrite<float>);
        lua_setfield(L, -2, "WriteFloat32");
        lua_pushcfunction(L, PtrWrite<double>);
        lua_setfield(L, -2, "WriteFloat64");

        lua_pushcfunction(L, UEScriptSDK::ReadAsciiStr);
        lua_setfield(L, -2, "ReadAsciiStr");
        lua_pushcfunction(L, UEScriptSDK::ReadWideStr);
        lua_setfield(L, -2, "ReadWideStr");

        lua_pushcfunction(L, UEScriptSDK::DerefPtr);
        lua_setfield(L, -2, "DerefPtr");

        lua_pushcfunction(L, UEScriptSDK::MemoryAlloc);
        lua_setfield(L, -2, "MemoryAlloc");

        lua_pushcfunction(L, UEScriptSDK::MemoryFree);
        lua_setfield(L, -2, "MemoryFree");

        lua_pushcfunction(L, UEScriptSDK::CurrentTimeUs);
        lua_setfield(L, -2, "CurrentTimeUs");

        lua_pushcfunction(L, UEScriptSDK::ResetLuaEngine);
        lua_setfield(L, -2, "ResetLuaEngine");
    }
    lua_setglobal(L, "sdk");
}

int UEScriptSDK::LoadString(lua_State* L)
{
    const char* chunk_name = luaL_checkstring(L, -2);
    const char* chunk = luaL_checkstring(L, -1);

    const bool ok = LuaEngine::LoadString(L, chunk_name, chunk);
    lua_pushboolean(L, ok);
    lua_insert(L, -2);
    return 2;
}

int UEScriptSDK::RunString(lua_State* L)
{
    const char* chunk_name = luaL_checkstring(L, -2);
    const char* code = luaL_checkstring(L, -1);

    const bool status = LuaEngine::RunString(L, chunk_name, code);
    lua_pushboolean(L, status);
    return 1;
}

int UEScriptSDK::DirIter(lua_State* L)
{
    const char* relative_path = luaL_checkstring(L, -1);

    if (!stdfs::exists(LuaEngine::GetHomeDirectory() / relative_path))
    {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    int i = 1;
    for (const auto& dir_entry : stdfs::recursive_directory_iterator(relative_path))
    {
        const std::string& path = dir_entry.path().string();
        lua_pushnumber(L, i);
        lua_pushstring(L, path.c_str());
        lua_settable(L, -3);
        i++;
    }

    return 1;
}

int UEScriptSDK::ReadAsciiStr(lua_State* L)
{
    const auto ptr = reinterpret_cast<const char*>(luaL_checkinteger(L, -1));
    TCheckPtrHot(ptr);
    lua_pushstring(L, ptr);
    return 1;
}

int UEScriptSDK::ReadWideStr(lua_State* L)
{
    const auto ptr = reinterpret_cast<const wchar_t*>(luaL_checkinteger(L, -1));
    TCheckPtrHot(ptr);
    const std::string_view& str = StringUtl::WideToAsciiStringFast(ptr, conv_buf::g_AsciiBuf);
    lua_pushstring(L, str.data());
    return 1;
}

int UEScriptSDK::DerefPtr(lua_State* L)
{
    const auto ptr = reinterpret_cast<void*>(luaL_checkinteger(L, -1));
    TCheckPtrHot(ptr);
#ifdef SCRIPT_SAFETY_ON
    if (IsBadReadPtr(ptr, sizeof(void*)))
    {
        return luaL_error(L, "bad read pointer");
    }
#endif
    const uintptr_t deref = *static_cast<uintptr_t*>(ptr);
    lua_pushinteger(L, static_cast<lua_Integer>(deref));
    return 1;
}

int UEScriptSDK::MemoryCopy(lua_State* L)
{
    const auto dst = reinterpret_cast<char*>(luaL_checkinteger(L, -3));
    TCheckPtrHot(dst);
    const char* source = reinterpret_cast<char*>(luaL_checkinteger(L, -2));
    TCheckPtrHot(source);
    const size_t count = luaL_checkinteger(L, -1);
    std::memcpy(dst, source, count);
    return 0;
}

int UEScriptSDK::MemoryAlloc(lua_State* L)
{
    const int bytes = static_cast<int>(luaL_checknumber(L, -1));

    auto memory = new char[bytes];
    std::memset(memory, 0, bytes);

    USafe(memory);

    g_AllocationTracker->AddAllocation(memory);

    lua_pushinteger(L, reinterpret_cast<lua_Integer>(memory));
    return 1;
}

int UEScriptSDK::MemoryFree(lua_State* L)
{
    if (const auto ptr = reinterpret_cast<char*>(lua_tointeger(L, -1)); g_AllocationTracker && ptr)
    {
        // No double-free.
        if (g_AllocationTracker->HasBeenFreed(ptr))
        {
            // It's Lua's fault if it tries to free this pointer again.
            g_AllocationTracker->RemoveAllocation(ptr);
            return 0;
        }

        g_AllocationTracker->SetFreed(ptr);
        delete[] ptr;
    }
    return 0;
}

int UEScriptSDK::CurrentTimeUs(lua_State* L)
{
    const auto& us
        = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    lua_pushinteger(L, us.count());
    return 1;
}

int UEScriptSDK::ResetLuaEngine(lua_State* L)
{
    const auto instance = LuaEngine::GetInstance(L);
    instance->m_IsResetPending = true;
    // Stop execution.
    return luaL_error(L, "Lua engine is resetting");
}
