#pragma once
#include <uescript.h>

#define TCheckPtr(v)                                                    \
	if (!(v)) [[unlikely]]                                                \
	{                                                                   \
		LuaSDK::PrintCallstack(L);                                      \
		return luaL_error(L, "%s: null pointer: %s", __FUNCTION__, #v); \
	}

#define SCRIPT_SAFETY_ON

#ifdef SCRIPT_SAFETY_ON
#define TCheckPtrHot(v) TCheckPtr(v)
#else
#define TCheckPtrHot(v)
#endif

struct lua_State;

class LuaSDK
{
public:
    LuaSDK() = default;
    virtual ~LuaSDK() = default;

    static void Init(lua_State* L);

protected:
    virtual void InitInternal(lua_State* L) = 0;

    static int Print(lua_State* L);
    static int Panic(lua_State* L);

    static void PrintCallstack(lua_State* L);
};

namespace sdk::util
{
    /**
     * @brief Casts the return value of Fn into a Lua-compatible integer.
     */
    template <auto Fn>
    constexpr int WrapPtr(lua_State* L)
    {
        auto ptr = Fn();
        if (!ptr)
        {
            lua_pushnil(L);
            return 1;
        }
        lua_pushinteger(L, reinterpret_cast<lua_Integer>(ptr));
        return 1;
    }

    template <class T, auto Field>
    constexpr int WrapField(lua_State* L)
    {
        const auto mem_fn = std::mem_fn(Field);
        const auto inst = reinterpret_cast<T*>(luaL_checkinteger(L, -1));
        UAssert(inst);
        const auto value = mem_fn(inst);
        // ReSharper disable once CppCStyleCast
        lua_pushinteger(L, (lua_Integer)value);
        return 1;
    }
}
