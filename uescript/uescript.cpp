#include "uescript.h"

#include <engine/engine.h>

#include <lua/lua_engine.h>
#include <utils/allocations.h>

constexpr int LUA_RESET_KEY = VK_F8;

UEScript::UEScript(const HMODULE module)
    : m_Module(module)
{
    m_GameWindow = FindWindowA("UnrealWindow", nullptr);
    UAssert(m_GameWindow && "Failed to find window. Is the class name not UnrealWindow?");

    InitConsole();

    g_AllocationTracker = std::make_unique<AllocationTracker>();
    m_LuaEngine = std::make_unique<LuaEngine>();

    InitHooks();
}

UEScript::~UEScript()
{
    Shutdown();
}

void UEScript::InitConsole()
{
    UAssert(AllocConsole());

    // Would prefer to write to the console directly but LuaJIT likes to print to std outputs.

    (void)freopen("CONIN$", "r", stdin);
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
    (void)std::setvbuf(stdout, nullptr, _IOFBF, 2048);
}

void UEScript::InitHooks()
{
    UAssert(m_LuaEngine);

    MH_STATUS status = MH_Initialize();
    UAssert(status == MH_OK);

    status = MH_CreateHook(
        g_EP.DrawTransition,
        static_cast<void*>(DrawTransition),
        reinterpret_cast<void**>(&g_EP.DrawTransition));
    UAssert(status == MH_OK);

    status = MH_CreateHook(
        g_EP.ProcessEvent,
        static_cast<void*>(ProcessEvent),
        reinterpret_cast<void**>(&g_EP.ProcessEvent));
    UAssert(status == MH_OK);

    status = MH_CreateHook(
        g_EP.FreeMemory,
        static_cast<void*>(FreeMemory),
        reinterpret_cast<void**>(&g_EP.FreeMemory));
    UAssert(status == MH_OK);

    status = MH_EnableHook(MH_ALL_HOOKS);
    UAssert(status == MH_OK);

    m_WndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
        m_GameWindow,
        GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(WndProc)));

    UAssert(m_WndProc && "SetWindowLongPtr failed");
}

void UEScript::Shutdown()
{
    MH_STATUS status = MH_DisableHook(MH_ALL_HOOKS);
    UAssert(status == MH_OK);
    status = MH_Uninitialize();
    UAssert(status == MH_OK);

    // Stupid.
    std::this_thread::sleep_for(chrono::milliseconds(200));

    g_AllocationTracker.reset(nullptr);

    m_LuaEngine.reset(nullptr);
    FreeConsole();
}

bool UEScript::AreHooksSafe()
{
    return g_UEScript && !s_IsUnloading && g_UEScript->m_LuaEngine && !g_UEScript->m_LuaEngine->IsResetPending();
}

void UEScript::DrawTransition(UObject* viewport_client, UObject* canvas)
{
    if (g_UEScript && !s_IsUnloading)
    {
        std::unique_ptr<LuaEngine>& engine = g_UEScript->m_LuaEngine;

        // Not sure if Lua initialization on the game thread is strictly necessary,
        // but it's probably a good idea.
        if (engine->IsInitializationPending())
        {
            engine->Initialize();
        }

        if (GetAsyncKeyState(LUA_RESET_KEY) || engine->IsResetPending())
        {
            // HACK: The LuaEngine destructor waits for the input thread to exit which will block until
            // enter is pressed.
            std::cout << "Lua reset is queued. Press enter in the console window." << std::endl;
            engine.reset(nullptr);
            std::cout << "Lua engine has been reset." << std::endl;
        }

        if (engine == nullptr)
        {
            std::cout << "Initializing Lua engine" << std::endl;
            engine = std::make_unique<LuaEngine>();
        }

        engine->DispatchDrawTransitionCallback(viewport_client, canvas);
    }

    return g_EP.DrawTransition(viewport_client, canvas);
}

void UEScript::ProcessEvent(UObject* object, UObject* function, void* params)
{
    if (AreHooksSafe())
    {
        bool should_call_original = true;
        g_UEScript->m_LuaEngine->DispatchProcessEventCallback(object, function, params, should_call_original);

        if (!should_call_original)
            return;
    }

    g_EP.ProcessEvent(object, function, params);
}

LRESULT __stdcall UEScript::WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if (AreHooksSafe())
    {
        bool should_call_original = true;
        g_UEScript->m_LuaEngine->DispatchWndProcCallback(umsg, wparam, lparam, should_call_original);

        if (!should_call_original)
            return 0;
    }

    return CallWindowProc(g_UEScript->m_WndProc, hwnd, umsg, wparam, lparam);
}

void UEScript::FreeMemory(void* memory)
{
    if (AreHooksSafe())
    {
        g_AllocationTracker->SetFreed(memory);
    }

    g_EP.FreeMemory(memory);
}

std::wstring_view UEScript::GetProcessName()
{
    static std::once_flag f;
    static std::wstring name(512, '\0');

    std::call_once(f, []
    {
        const DWORD status = GetModuleFileNameW(GetModuleHandleA(nullptr),
                                                name.data(), static_cast<DWORD>(name.size() - 1));
        UAssert(status != 0);

        name = stdfs::path(name).filename();
    });

    return name;
}

void UEScript::AssertionFailure(const char* message, const std::source_location& location)
{
    const std::string& formatted_msg = std::format(
        "Condition: {}\nFunction: {}\nFile: {}:{}", message, location.function_name(), location.file_name(),
        location.line());
#ifdef _DEBUG
    __debugbreak();
#endif
    MessageBoxA(nullptr, formatted_msg.c_str(), "Assertion Failed", MB_OK | MB_ICONERROR);
    ExitProcess(0);
}
