#include <uescript.h>
#include "signature.h"

#define INRANGE(x, a, b) (x >= a && x <= b)

#define GET_BITS(x)                                                 \
	(INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xa) \
									  : (INRANGE(x, '0', '9') ? x - '0' : 0))
#define GET_BYTE(x) (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))

std::pair<uintptr_t, uintptr_t> GetModuleSizeAddr(const char* module)
{
    const uintptr_t handle = reinterpret_cast<uintptr_t>(GetModuleHandleA(module));
    UAssert(handle);

    const auto doshdr = reinterpret_cast<PIMAGE_DOS_HEADER>(handle);
    UAssert(doshdr);

    const auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(handle + doshdr->e_lfanew);
    UAssert(nt);

    uintptr_t address = handle + nt->OptionalHeader.BaseOfCode;
    uintptr_t size = address + nt->OptionalHeader.SizeOfCode;

    return {address, size};
}

int Signature::Find(lua_State* L)
{
    const char* module = nullptr;
    // Optional parameter
    if (lua_gettop(L) >= 2 && lua_isstring(L, -2))
        module = lua_tostring(L, -2);

    const auto [address, size] = GetModuleSizeAddr(module);

    const char* pattern = luaL_checkstring(L, -1);
    const char* pat = pattern;

    uintptr_t first_match = 0;

    for (uintptr_t cur = address; cur < size; cur++)
    {
        if (!*pat)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(first_match));
            return 1;
        }

        if (*(uint8_t*)pat == '\?' || *(uint8_t*)cur == GET_BYTE(pat))
        {
            if (!first_match)
                first_match = cur;

            if (!pat[2])
            {
                lua_pushinteger(L, static_cast<lua_Integer>(first_match));
                return 1;
            }

            if (*(uint16_t*)pat == '\?\?' || *(uint8_t*)pat != '\?')
                pat += 3;
            else
                pat += 2;
        }
        else
        {
            pat = pattern;
            first_match = 0;
        }
    }

    lua_pushnil(L);
    return 1;
}

int Signature::Rip(lua_State* L)
{
    const uintptr_t address = luaL_checkinteger(L, -3);
    const int Offset = static_cast<int>(luaL_checknumber(L, -2));
    const int OpSize = static_cast<int>(luaL_checknumber(L, -1));

    const int32_t rip = *reinterpret_cast<int32_t*>(address + Offset);
    const uintptr_t abs = address + OpSize + rip;

    lua_pushinteger(L, static_cast<lua_Integer>(abs));
    return 1;
}
