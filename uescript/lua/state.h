#pragma once
#include <uescript.h>

struct lua_State;

/**
 * @brief Stores a lua_State pointer and a mutex for thread-safe access.
 */
class LuaState final
{
    friend class StateView;
    friend class StateLock;

public:
    explicit LuaState();
    ~LuaState();

    // No copy constructors.

    LuaState& operator=(const LuaState&) = delete;
    LuaState(const LuaState&) = delete;

    FORCEINLINE operator lua_State*() const
    {
        return m_State;
    }

private:
    std::recursive_mutex m_Mutex{};
    lua_State* m_State{nullptr};
};

/**
 * @brief A view into a LuaState (cannot use its lock).
 */
class StateView final
{
public:
    // No default constructor.
    StateView() = delete;

    FORCEINLINE StateView(lua_State* state)
        : m_State{state}
    {
    }

    FORCEINLINE StateView(const LuaState& state)
        : m_State{state}
    {
    }

    FORCEINLINE operator lua_State*() const
    {
        return m_State;
    }

private:
    lua_State* m_State{nullptr};
};

/**
 * @brief Acts as a scoped lock for a LuaState, locking its mutex on construction and unlocking it on destruction.
 */
class StateLock final
{
public:
    // No default constructor.
    StateLock() = delete;
    // No copy constructors.
    StateLock& operator=(const StateLock&) = delete;
    StateLock(const StateLock&) = delete;

    FORCEINLINE explicit StateLock(LuaState& state)
        : m_State{state}
    {
        m_State.m_Mutex.lock();
    }

    FORCEINLINE ~StateLock()
    {
        m_State.m_Mutex.unlock();
    }

    FORCEINLINE operator lua_State*() const
    {
        return m_State;
    }

    FORCEINLINE operator StateView() const
    {
        return m_State;
    }

private:
    LuaState& m_State;
};

/**
 * @brief Ensures the Lua stack top is the same at destruction as it was upon construction.
 */
class StackGuard final
{
public:
    explicit StackGuard(StateView L) : m_State{L},
                                       m_Top{lua_gettop(m_State)}
    {
    }

    ~StackGuard()
    {
        const int items = lua_gettop(m_State) - m_Top;
        lua_pop(m_State, items);
    }

private:
    StateView m_State;
    int m_Top{-1};
};

/**
 * @brief Stores a Lua reference along with its state. Both the state and reference values may be invalid.
 */
class LuaRef final
{
public:
    explicit LuaRef() = default;

    explicit LuaRef(StateView state, int reference)
        : m_State{state}, m_Ref{reference}
    {
    }

    ~LuaRef()
    {
        Unref();
    }

    /**
     * @brief Update the reference's state pointer and reference value.
     */
    void Set(const StateView state, const int reference)
    {
        m_State = state;
        m_Ref = reference;
    }

    /**
     * @return Whether the reference is valid.
     */
    bool Valid() const
    {
        return m_State && m_Ref != LUA_REFNIL && m_Ref != LUA_NOREF;
    }

    /**
     * @return The raw reference value.
     */
    int Reference() const
    {
        return m_Ref;
    }

    /**
     * @brief Push the references value onto the stack.
     */
    void Push() const
    {
        lua_rawgeti(m_State, LUA_REGISTRYINDEX, m_Ref);
    }

    /**
     * @brief Frees the reference value.
     */
    void Unref()
    {
        if (m_State && Valid())
            luaL_unref(m_State, LUA_REGISTRYINDEX, m_Ref);
        m_Ref = LUA_NOREF;
    }

private:
    StateView m_State{nullptr};
    int m_Ref{LUA_NOREF};
};
