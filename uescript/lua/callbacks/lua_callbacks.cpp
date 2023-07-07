#include <uescript.h>
#include "lua_callbacks.h"
#include <lua/lua_engine.h>

std::initializer_list<std::pair<CallbackId, const char*>> Callbacks = {
    {CallbackId::DrawTransition, "_CallbackDrawTransition"},
    {CallbackId::ProcessEvent, "_CallbackProcessEvent"},
    {CallbackId::WndProc, "_CallbackWndProc"}
};

LuaCallbacks::LuaCallbacks(StateView L)
{
    lua_newtable(L);
    {
        lua_pushcfunction(L, RefreshCallbackReferences);
        lua_setfield(L, -2, "RefreshCallbackReferences");
    }
    lua_setglobal(L, "_callbacks");
}

LuaCallbacks::~LuaCallbacks()
{
    ClearReferences();
}

const LuaRef& LuaCallbacks::GetReference(CallbackId id) const
{
    const LuaRef& slot = m_Slots.at(static_cast<size_t>(id));
    return slot;
}

void LuaCallbacks::ClearReferences()
{
    for (LuaRef& slot : m_Slots)
        slot.Unref();
}

int LuaCallbacks::RefreshCallbackReferences(lua_State* L)
{
    LuaEngine* engine = LuaEngine::GetInstance(L);

    LuaCallbacks& callbacks = engine->m_Callbacks;
    callbacks.ClearReferences();

    for (const auto& [callback_id, name] : Callbacks)
    {
        const StackGuard guard(L);

        lua_getglobal(L, name);
        if (lua_isfunction(L, -1))
        {
            const int reference = luaL_ref(L, LUA_REGISTRYINDEX);

            LuaRef& ref = callbacks.m_Slots.at(static_cast<size_t>(callback_id));
            ref.Set(L, reference);

            UAssert(ref.Valid());
        }
    }

    return 0;
}
