#pragma once
#include "state.h"

/**
 * @brief Helper class for LuaEngine that handles Lua bootstrapping.
 */
class LuaBootstrap final
{
    friend class LuaEngine;

    LuaBootstrap() = default;
    ~LuaBootstrap() = default;

    /**
     * @brief Bootstrap Lua. This sets up the global Unreal Engine pointers.
     * @return Whether bootstrapping was successful
     */
    bool Bootstrap() const;

    /**
     * @brief Write a sample bootstrap file.
     */
    static void WriteSampleBootstrap();

    LuaState m_State{};
    static inline bool s_HasBootstrapped{false};
};
