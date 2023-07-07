#pragma once
struct lua_State;

/**
 * @brief Signature scanning.
 */
class Signature final
{
public:
    Signature() = delete;
    /**
     * @brief Scans for a signature and pushes the address or nil if it could not be found.
     */
    static int Find(lua_State* L);
    /**
     * @brief Convert a relative pointer to absolute and pushes the result.
     */
    static int Rip(lua_State* L);
};
