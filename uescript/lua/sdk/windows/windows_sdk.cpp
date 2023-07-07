#include <uescript.h>
#include "windows_sdk.h"

void WindowsSDK::InitInternal(lua_State* L)
{
    lua_newtable(L);
    {
        lua_pushcfunction(L, WindowsSDK::GetCursorPos);
        lua_setfield(L, -2, "GetCursorPos");
        lua_pushcfunction(L, WindowsSDK::GetModuleAddress);
        lua_setfield(L, -2, "GetModuleAddress");
        lua_pushcfunction(L, WindowsSDK::VirtualProtect);
        lua_setfield(L, -2, "VirtualProtect");
    }
    lua_setglobal(L, "windows");
}

int WindowsSDK::GetModuleAddress(lua_State* L)
{
    const char* name = lua_tostring(L, -1);
    const uint64_t address = reinterpret_cast<uint64_t>(GetModuleHandleA(name));
    lua_pushinteger(L, static_cast<lua_Integer>(address));
    return 1;
}

int WindowsSDK::VirtualProtect(lua_State* L)
{
    const auto ptr = reinterpret_cast<void*>(lua_tointeger(L, -3));
    const size_t size = static_cast<size_t>(lua_tointeger(L, -2));
    const int new_protect = static_cast<int>(lua_tointeger(L, -1));
    DWORD old_protect = 0;
    const auto status = ::VirtualProtect(ptr, size, new_protect, &old_protect);
    lua_pushinteger(L, status);
    lua_pushinteger(L, old_protect);
    return 2;
}

int WindowsSDK::GetCursorPos(lua_State* L)
{
    POINT p{0, 0};
    if (::GetCursorPos(&p) == 0)
    {
        lua_pushnil(L);
        return 1;
    }

    if (ScreenToClient(g_UEScript->GetGameWindow(), &p) == 0)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    return 2;
}
