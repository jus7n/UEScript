#include <uescript.h>
#include "state.h"

LuaState::LuaState()
    : m_State{lua_open()}
{
    UAssert(m_State);
}

LuaState::~LuaState()
{
    std::scoped_lock lock(m_Mutex);
    lua_close(m_State);
    m_State = nullptr;
}
