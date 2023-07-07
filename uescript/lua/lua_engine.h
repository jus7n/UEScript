#pragma once
#include <uescript.h>
#include "state.h"
#include "callbacks/lua_callbacks.h"

struct UObject;

/**
 * @brief Manages Lua execution and callbacks.
 */
class LuaEngine final
{
    friend class UEScriptSDK;
    friend class LuaCallbacks;
    friend class LuaBootstrap;

public:
    explicit LuaEngine();
    ~LuaEngine();

    /**
     * @return Whether initialization is pending
     */
    bool IsInitializationPending()
    {
        return m_IsInitializationPending;
    }

    /**
     * @return Whether reset is pending
    */
    bool IsResetPending()
    {
        return m_IsResetPending;
    }

    /**
     * @brief Initializes the engine. Must be called on a game thread.
     */
    void Initialize();

private:
    /**
     * @brief Begins Lua execution.
     */
    void Startup() const;

    /**
     * @brief Compile a Lua chunk, leaving the compiled function on the Lua stack.
     * @param chunk_name The name of the chunk
     * @param chunk Chunk source/bytecode
     * @return true if compilation succeeded, false otherwise
     */
    static bool LoadString(StateView L, const std::string_view& chunk_name, const std::string_view& chunk);

    /**
     * @brief Compile and execute a Lua chunk.
     * @param chunk_name The name of the chunk
     * @param chunk Chunk source/bytecode
     * @return true if execution succeeded, false otherwise
     */
    static bool RunString(StateView L, const std::string_view& chunk_name, const std::string_view& chunk);

    /**
     * @brief Print a Lua status message. Assumes in certain cases that the top of the stack contains an error string.
     */
    static void PrintStatus(StateView L, const std::string_view& chunk_name, int status);

    /**
     * @brief Handles input and dispatching to Lua.
     */
    int InputThread();

    /**
     * @brief Read a file from the home directory.
     * @param path A relative or absolute path to the file. Relative paths are resolved within the home directory.
     * @return An optional value containing the file contents
     */
    static std::optional<std::string> ReadFile(const std::string_view& path);

    /**
     * @brief Resolve a path within the home directory. Absolute paths are returned as-is.
     * @param path The path to resolve
     * @return The resolved path
     */
    static stdfs::path ResolvePath(const stdfs::path& path);

    /**
     * @brief Retrieve the engine instance from the registry.
     * @param L The Lua state
     * @return The engine instance
     */
    static LuaEngine* GetInstance(StateView L);

public:
    /**
     * @return The directory in which Lua is loaded from.
     */
    static stdfs::path GetHomeDirectory();

    /**
     * @brief Writes a sample startup.lua file. 
     */
    static void WriteDefaultStartupFile();

    /**
     * @brief Invoke the DrawTransition Lua callback.
     */
    void DispatchDrawTransitionCallback(UObject* viewport_client, UObject* canvas);

    /**
     * @brief Invoke the ProcessEvent Lua callback.
     * @param should_call_original Whether or not to call the original function
     */
    void DispatchProcessEventCallback(UObject* object, UObject* function, void* params, bool& should_call_original);

    /**
     * @brief Invoke the WndProc Lua callback.
     * @param should_call_original Whether or not to call the original function
     */
    void DispatchWndProcCallback(UINT umsg, WPARAM wparam, LPARAM lparam, bool& should_call_original);

private:
    std::atomic_bool m_Exiting{false};
    std::atomic_bool m_IsInitializationPending{true};
    std::atomic_bool m_IsResetPending{false};

    bool m_BootstrapComplete{false};

    std::thread m_InputThread{};

    LuaState m_State{};

    LuaCallbacks m_Callbacks;
};
