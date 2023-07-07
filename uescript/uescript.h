#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#undef LoadString
#undef ProcessEvent
#undef DrawText

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <thread>
#include <unordered_set>
#include <vector>
#include <source_location>

#include <utils/str.h>

#include <MinHook.h>

#include <lua.hpp>
static_assert(sizeof(lua_Integer) == sizeof(void*));

namespace stdfs = std::filesystem;
namespace chrono = std::chrono;

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

#ifndef FORCENOINLINE
#define FORCENOINLINE __declspec(noinline)
#endif

#define UAssert(cond)                                       \
	if (!(cond)) [[unlikely]]                               \
	{                                                       \
		UEScript::AssertionFailure(#cond, std::source_location::current()); \
	}

// Enable Lua safety features
#define SCRIPT_SAFETY_ON

#ifdef SCRIPT_SAFETY_ON
#define USafe(cond) UAssert(cond)
#else
#define USafe(cond)
#endif

class UEScript final
{
public:
    explicit UEScript(HMODULE module);
    ~UEScript();

    // No copy constructors.
    UEScript& operator=(const UEScript&) = delete;
    UEScript(const UEScript&) = delete;

    HMODULE GetModule() const { return m_Module; }
    HWND GetGameWindow() const { return m_GameWindow; }

    static std::wstring_view GetProcessName();

    FORCENOINLINE static void AssertionFailure(const char* message, const std::source_location& location);

private:
    void InitConsole();
    void InitHooks();
    void Shutdown();

    static bool AreHooksSafe();

    static void DrawTransition(struct UObject* viewport_client, struct UObject* canvas);
    static void ProcessEvent(struct UObject* object, struct UObject* function, void* params);
    static LRESULT __stdcall WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
    static void FreeMemory(void* memory);

    HWND m_GameWindow{nullptr};
    HMODULE m_Module{nullptr};
    WNDPROC m_WndProc{nullptr};
    std::unique_ptr<class LuaEngine> m_LuaEngine{nullptr};

public:
    static inline std::atomic_bool s_IsUnloading{false};
};

inline std::unique_ptr<UEScript> g_UEScript{nullptr};
