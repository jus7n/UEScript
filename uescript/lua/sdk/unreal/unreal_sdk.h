#pragma once
#include "../sdk.h"

/**
 * @brief SDK for interacting with Unreal Engine objects.
 */
class UnrealSDK final : public LuaSDK
{
public:
    UnrealSDK() = default;
    ~UnrealSDK() override = default;

protected:
    void InitInternal(lua_State* L) override;

private:
    /**
     * @brief Expose Unreal Engine types to Lua.
     */
    static void GenerateUnrealTypes(lua_State* L);

    static int WorldToScreen(lua_State* L);
    static int DrawText(lua_State* L);
    static int DrawLine(lua_State* L);
    static int DrawRect(lua_State* L);
    static int DrawFilledRect(lua_State* L);
    static int SizeOfText(lua_State* L);

    static int StaticFindObject(lua_State* L);
    static int FindObjectSlow(lua_State* L);
    static int FindAllObjectsSlow(lua_State* L);
    static int GetAllActorsOfClass(lua_State* L);

    static int FNameToString(lua_State* L);

    static int ProcessEvent(lua_State* L);
    static int GetObjectName(lua_State* L);
};
