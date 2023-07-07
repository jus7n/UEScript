#pragma once
#include <uescript.h>
#include <lua/state.h>

enum class CallbackId : uint8_t
{
    DrawTransition,
    ProcessEvent,
    WndProc,
    // Must be last
    Max,
};

/**
 * @brief Manages references to Lua callback functions.
 */
class LuaCallbacks final
{
    friend class LuaEngine;

    explicit LuaCallbacks(StateView L);
    ~LuaCallbacks();

    /**
     * @brief Retrieves a reference from a callback id. The returned reference, if valid, is guaranteed to be a function.
     */
    const LuaRef& GetReference(CallbackId id) const;

    /**
     * @brief Clears and unreferences callbacks.
     */
    void ClearReferences();

    /**
     * @brief Clears and refreshes callback references.
     */
    static int RefreshCallbackReferences(lua_State* L);

    std::array<LuaRef, static_cast<size_t>(CallbackId::Max)> m_Slots;
};
