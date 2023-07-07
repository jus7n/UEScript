#pragma once
#include "../sdk.h"

/**
 * @brief SDK for interacting with the Windows API.
 */
class WindowsSDK final : public LuaSDK
{
public:
    WindowsSDK() = default;
    ~WindowsSDK() override = default;

protected:
    void InitInternal(lua_State* L) override;

public: // Available for bootstrap to use
    static int GetModuleAddress(lua_State* L);

private:
    static int VirtualProtect(lua_State* L);
    static int GetCursorPos(lua_State* L);
};
