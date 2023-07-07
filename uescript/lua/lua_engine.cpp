#include <uescript.h>
#include "lua_engine.h"
#include "bootstrap.h"

#include "sdk/sdk.h"
#include <engine/engine.h>
#include <ShlObj.h>

constexpr const char* STARTUP_FILE = "startup.lua";
constexpr int ENGINE_REF = 1;

LuaEngine::LuaEngine() : m_Callbacks(m_State)
{
    const LuaBootstrap bootstrap;
    const bool success = bootstrap.Bootstrap();
    UAssert(success && "Lua bootstrap failed. Please check the console for details.");

    m_BootstrapComplete = success;
}

LuaEngine::~LuaEngine()
{
    UAssert(m_BootstrapComplete);
    UAssert(m_IsInitializationPending == false);
    m_Exiting = true;
    m_InputThread.join();
}

void LuaEngine::Initialize()
{
    UAssert(m_BootstrapComplete);
    m_IsInitializationPending = false;

    const StateLock lock(m_State);

    const auto ud = static_cast<LuaEngine**>(lua_newuserdata(lock, sizeof(LuaEngine*)));
    *ud = this;

    const int reference = luaL_ref(lock, LUA_REGISTRYINDEX);

    // The reference to our own instance should always be the first created.
    UAssert(ENGINE_REF == reference);

    LuaSDK::Init(lock);

    m_InputThread = std::thread(&LuaEngine::InputThread, this);

    Startup();
}

void LuaEngine::Startup() const
{
    const auto& startup_contents = ReadFile(STARTUP_FILE);
    if (!startup_contents.has_value())
    {
        std::wcout << "Cannot run " << STARTUP_FILE << " because it does not exist in " << GetHomeDirectory() <<
            std::endl;
        WriteDefaultStartupFile();
        std::wcout << "A sample " << STARTUP_FILE << " has been written to " << GetHomeDirectory() / STARTUP_FILE <<
            std::endl;
        return;
    }

    if (!LoadString(m_State, "uescript:startup", startup_contents.value()))
        return;

    lua_pushstring(m_State, StringUtl::WideToAsciiString(UEScript::GetProcessName()).c_str());

    if (const int status = lua_pcall(m_State, 1, 0, 0); status != LUA_OK)
    {
        PrintStatus(m_State, "uescript:startup", status);
    }
}

bool LuaEngine::LoadString(StateView L, const std::string_view& chunk_name, const std::string_view& chunk)
{
    if (const int status = luaL_loadbuffer(L, chunk.data(), chunk.size(), chunk_name.data()); status != LUA_OK)
    {
        PrintStatus(L, chunk_name, status);
        return false;
    }

    return true;
}

bool LuaEngine::RunString(StateView L, const std::string_view& chunk_name, const std::string_view& chunk)
{
    if (!LoadString(L, chunk_name, chunk))
        return false;

    if (const int status = lua_pcall(L, 0, 0, 0); status != LUA_OK)
    {
        PrintStatus(L, chunk_name, status);
        return false;
    }

    return true;
}

void LuaEngine::PrintStatus(StateView L, const std::string_view& chunk_name, const int status)
{
    switch (status)
    {
    case LUA_ERRMEM:
        std::cerr << "LUA ERROR: Memory allocation failure running: " << chunk_name << std::endl;
        break;
    case LUA_ERRSYNTAX:
        {
            const char* message = lua_tostring(L, -1);
            std::cerr << "LUA ERROR: Failed to compile " << chunk_name << ": " << message << std::endl;
            lua_pop(L, 1);
            break;
        }
    case LUA_ERRRUN:
        {
            const char* message = lua_tostring(L, -1);
            std::cerr << "LUA ERROR: Failed to run " << chunk_name << ": " << message << std::endl;
            lua_pop(L, 1);
            break;
        }
    case LUA_ERRERR:
        std::cerr << "LUA ERROR: Error running error handling function for " << chunk_name << std::endl;
        break;
    default:
        // No error.
        break;
    }

    lua_pop(L, 1);
}

int LuaEngine::InputThread()
{
    std::string input;

    while (!m_Exiting)
    {
        std::getline(std::cin, input);

        const StateLock lock(m_State);
        const StackGuard guard(lock);

        lua_getglobal(lock, "_CallbackInput");
        if (!lua_isfunction(lock, -1))
            continue;

        lua_pushstring(lock, input.c_str());

        if (const int status = lua_pcall(lock, 1, 0, 0); status != LUA_OK)
        {
            PrintStatus(lock, "uescript:input", status);
        }
    }

    return 0;
}

std::optional<std::string> LuaEngine::ReadFile(const std::string_view& path)
{
    const auto& absolute_path = ResolvePath(path);
    if (!stdfs::exists(absolute_path))
        return {};

    std::ifstream in(absolute_path);

    const std::istreambuf_iterator it(in);
    std::string data(it, std::istreambuf_iterator<char>());

    return data;
}

stdfs::path LuaEngine::ResolvePath(const stdfs::path& path)
{
    if (!path.is_absolute())
    {
        return GetHomeDirectory() / path;
    }
    return path;
}

stdfs::path LuaEngine::GetHomeDirectory()
{
    static stdfs::path home;
    static std::once_flag f;

    std::call_once(f, []
    {
        wchar_t* path;
        const auto res = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &path);
        UAssert(SUCCEEDED(res));
        home = stdfs::path(path) / "uescript/";
        CoTaskMemFree(path);

        if (!stdfs::exists(home))
        {
            stdfs::create_directories(home);
            std::wcout << L"Created " << home << std::endl;
        }
    });

    return home;
}

void LuaEngine::WriteDefaultStartupFile()
{
    std::ofstream out(GetHomeDirectory() / STARTUP_FILE);
    out << "local process_name = ...";
    out << "print('Hello from startup: ' .. process_name)";
}

LuaEngine* LuaEngine::GetInstance(StateView L)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, ENGINE_REF);
    LuaEngine* inst = *static_cast<LuaEngine**>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return inst;
}

void LuaEngine::DispatchDrawTransitionCallback(UObject* viewport_client, UObject* canvas)
{
    const StateLock lock(m_State);
    const StackGuard guard(lock);

    if (const auto& reference = m_Callbacks.GetReference(CallbackId::DrawTransition); reference.Valid())
    {
        // Reference is always going to be a function.
        reference.Push();

        lua_pushinteger(lock, reinterpret_cast<lua_Integer>(viewport_client));
        lua_pushinteger(lock, reinterpret_cast<lua_Integer>(canvas));

        if (const int status = lua_pcall(lock, 2, 0, 0); status != LUA_OK)
        {
            PrintStatus(lock, "uescript:draw_transition", status);
        }
    }
}

void LuaEngine::DispatchProcessEventCallback(UObject* object, UObject* function, void* params,
                                             bool& should_call_original)
{
    const StateLock lock(m_State);
    const StackGuard guard(lock);

    if (const auto& reference = m_Callbacks.GetReference(CallbackId::ProcessEvent); reference.Valid())
    {
        // Reference is always going to be a function.
        reference.Push();

        lua_pushinteger(lock, reinterpret_cast<lua_Integer>(object));
        lua_pushinteger(lock, reinterpret_cast<lua_Integer>(function));
        lua_pushinteger(lock, reinterpret_cast<lua_Integer>(params));

        if (const int status = lua_pcall(lock, 3, 1, 0); status != LUA_OK)
        {
            PrintStatus(lock, "uescript:process_event", status);
            return;
        }

        if (!lua_isboolean(lock, -1))
            return;

        const bool call_result = lua_toboolean(lock, -1);

        should_call_original = call_result;
    }
}

void LuaEngine::DispatchWndProcCallback(UINT umsg, WPARAM wparam, LPARAM lparam, bool& should_call_original)
{
    const StateLock lock(m_State);
    const StackGuard guard(lock);

    if (const auto& reference = m_Callbacks.GetReference(CallbackId::WndProc); reference.Valid())
    {
        // Reference is always going to be a function.
        reference.Push();

        lua_pushinteger(lock, umsg);
        lua_pushinteger(lock, static_cast<lua_Integer>(wparam));
        lua_pushinteger(lock, lparam);

        if (const int status = lua_pcall(lock, 3, 1, 0); status != LUA_OK)
        {
            PrintStatus(lock, "uescript:wndproc", status);
            return;
        }

        if (!lua_isboolean(lock, -1))
            return;

        const bool call_result = lua_toboolean(lock, -1);

        should_call_original = call_result;
    }
}
