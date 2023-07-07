#pragma once
#include "../sdk.h"

/**
 * @brief SDK for signature scanning.
 */
class SignatureSDK final : public LuaSDK
{
public:
    SignatureSDK() = default;
    ~SignatureSDK() override = default;

protected:
    void InitInternal(lua_State* L) override;
};
