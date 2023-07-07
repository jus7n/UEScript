#include <uescript.h>
#include "signature_sdk.h"

#include <utils/signature.h>

void SignatureSDK::InitInternal(lua_State* L)
{
    lua_newtable(L);
    {
        lua_pushcfunction(L, Signature::Find);
        lua_setfield(L, -2, "Find");
        lua_pushcfunction(L, Signature::Rip);
        lua_setfield(L, -2, "Rip");
    }
    lua_setglobal(L, "sig");
}
