#include <uescript.h>
#include "bootstrap.h"

#include <ranges>

#include "lua_engine.h"

#include "sdk/windows/windows_sdk.h"

#include <utils/signature.h>
#include <engine/engine.h>

static constexpr const char* BOOTSTRAP_FILE = "bootstrap.lua";

#define FIELD(name)                           \
	{                                         \
		#name, offsetof(EnginePointers, name) \
	}
static std::initializer_list<std::pair<const char*, uint64_t>> BootstrapFields = {
    FIELD(ProcessEvent),
    FIELD(DrawTransition),
    FIELD(FreeMemory),
    FIELD(GetObjectName),
    FIELD(StaticFindObject),
    FIELD(GetAllActorsOfClass),
    FIELD(FNameToString),
    FIELD(WorldToScreen),
    FIELD(DrawText),
    FIELD(DrawLine),
    FIELD(DrawFilledRect),
    FIELD(SizeOfText),
    FIELD(UObjectArray),
    FIELD(UWorld),
    FIELD(UEngine),
    FIELD(GNames),
};
static_assert(sizeof(EnginePointers) == sizeof(void*) * 16, "Don't forget to update the bootstrap fields!");
#undef FIELD

bool LuaBootstrap::Bootstrap() const
{
    // Skip bootstrap if the engine pointers have already been initialized
    if (s_HasBootstrapped)
    {
        return true;
    }

    const std::optional<std::string>& bootstrap_contents = LuaEngine::ReadFile(BOOTSTRAP_FILE);
    if (!bootstrap_contents.has_value())
    {
        std::wcout << "Cannot run " << BOOTSTRAP_FILE << " because it does not exist in " <<
            LuaEngine::GetHomeDirectory() << std::endl;
        WriteSampleBootstrap();
        std::wcout << "A sample " << BOOTSTRAP_FILE << " has been written to " << LuaEngine::GetHomeDirectory() /
            BOOTSTRAP_FILE << std::endl;
        return false;
    }

    const auto& then = chrono::system_clock::now();

    const LuaState& L = m_State;
    luaL_openlibs(L);

    lua_newtable(L);
    {
        lua_pushcfunction(L, Signature::Find);
        lua_setfield(L, -2, "Find");
        lua_pushcfunction(L, Signature::Rip);
        lua_setfield(L, -2, "Rip");
    }
    lua_setglobal(L, "sig");

    lua_newtable(L);
    {
        lua_pushcfunction(L, WindowsSDK::GetModuleAddress);
        lua_setfield(L, -2, "GetModuleAddress");
    }
    lua_setglobal(L, "windows");

    if (!LuaEngine::LoadString(L, "uescript:bootstrap", bootstrap_contents.value()))
    {
        return false;
    }

    lua_pushstring(L, StringUtl::WideToAsciiString(UEScript::GetProcessName()).c_str());

    if (const int status = lua_pcall(L, 1, 1, 0); status != LUA_OK)
    {
        LuaEngine::PrintStatus(L, "uescript:bootstrap", status);
        return false;
    }

    if (lua_type(L, -1) != LUA_TTABLE)
    {
        std::cerr << "Expected bootstrap chunk to return table" << std::endl;
        lua_pop(L, -1);
        return false;
    }

    for (const auto& [name, offset] : BootstrapFields)
    {
        const StackGuard guard(L);

        lua_getfield(L, -1, name);
        UAssert(lua_type(L, -1) == LUA_TNUMBER && "Bootstrap result is not a number");

        const auto value = reinterpret_cast<void*>(lua_tointeger(L, -1));
        const auto dest = reinterpret_cast<void**>(reinterpret_cast<uint64_t>(&g_EP) + offset);

        if (!value)
        {
            std::cerr << "Invalid bootstrap pointer: " << name << ": " << std::hex << value << std::dec << std::endl;
            return false;
        }

        // Update the field at the given offset within EnginePointers
        *dest = value;

#ifdef _DEBUG
        std::cout << name << ": " << std::hex << value << std::dec << std::endl;
#endif
    }

    const auto& delta = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - then);
    std::cout << "Bootstrap done in " << delta.count() << " ms" << std::endl;

    s_HasBootstrapped = true;
    return true;
}

void LuaBootstrap::WriteSampleBootstrap()
{
    std::ofstream out(LuaEngine::GetHomeDirectory() / BOOTSTRAP_FILE);
    out << R"(
-- The name of the process is passed to the bootstrap chunk
local process_name = ...
assert(process_name == "ExampleGame-Win64-Shipping.exe")

--[[
	To retrieve the required pointers, you may use the signature library:
		- sig.Find("IDA-style signature")
		- sig.Find("MyModule.dll", "IDA-style signature")
	
	Converting a relative pointer:
		- sig.Rip(relative_pointer, offset, opcode_size)
	
	Getting the base address of a module:
		- windows.GetModuleAddress("MyModule.dll")
	
	The LuaJIT ffi library is also available to use.
]]--

return {
)";
    for (const auto& name : BootstrapFields | std::views::keys)
        out << "\t" << name << " = 0,\n";
    out << "}";
}
