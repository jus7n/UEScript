#include <uescript.h>

DWORD MainThread(void* module)
{
    g_UEScript = std::make_unique<UEScript>(static_cast<HMODULE>(module));

    while (!UEScript::s_IsUnloading)
    {
        std::this_thread::sleep_for(chrono::milliseconds(500));
    }

    g_UEScript.reset(nullptr);

    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    (void)lpReserved;
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        return TRUE;
    }

    return FALSE;
}
