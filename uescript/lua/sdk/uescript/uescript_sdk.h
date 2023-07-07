#pragma once
#include "../sdk.h"

/**
 * @brief SDK for interacting with UEScript itself and doing memory operations.
 */
class UEScriptSDK final : public LuaSDK
{
public:
    UEScriptSDK() = default;
    ~UEScriptSDK() override = default;

protected:
    void InitInternal(lua_State* L) override;

private:
    // ReSharper disable CppCStyleCast

    template <typename T>
    static constexpr int PtrWrite(lua_State* L)
    {
        T* ptr = (T*)luaL_checkinteger(L, -2);
        TCheckPtrHot(ptr);
        const T value = (T)luaL_checkinteger(L, -1);
        *ptr = value;
        return 0;
    }

    template <typename T>
    static constexpr int PtrRead(lua_State* L)
    {
        const T* ptr = (const T*)luaL_checkinteger(L, -1);
        TCheckPtrHot(ptr);
        // Only push a number for floating point types.
        if constexpr (std::is_floating_point_v<T>)
        {
            lua_pushnumber(L, (lua_Number)*ptr);
        }
        else
        {
            lua_pushinteger(L, (lua_Integer)*ptr);
        }
        return 1;
    }

    template <typename T>
    static constexpr void DefineIntPtrOp(lua_State* L)
    {
        static_assert(std::is_signed_v<T>, "expected signed type");
        using UT = std::make_unsigned_t<T>;

        constexpr auto size = sizeof(T) * 8;

        // Making these formats constexpr isn't worth it.
        lua_pushcfunction(L, PtrRead<T>);
        lua_setfield(L, -2, std::format("ReadI{}", size).c_str());
        lua_pushcfunction(L, PtrWrite<T>);
        lua_setfield(L, -2, std::format("WriteI{}", size).c_str());

        lua_pushcfunction(L, PtrRead<UT>);
        lua_setfield(L, -2, std::format("ReadU{}", size).c_str());
        lua_pushcfunction(L, PtrWrite<UT>);
        lua_setfield(L, -2, std::format("WriteU{}", size).c_str());
    }

    // ReSharper restore CppCStyleCast

    static int LoadString(lua_State* L);
    static int RunString(lua_State* L);

    static int DirIter(lua_State* L);

    static int ReadAsciiStr(lua_State* L);
    static int ReadWideStr(lua_State* L);

    static int DerefPtr(lua_State* L);

    static int MemoryCopy(lua_State* L);

    static int MemoryAlloc(lua_State* L);
    static int MemoryFree(lua_State* L);

    static int CurrentTimeUs(lua_State* L);

    static int ResetLuaEngine(lua_State* L);
};
