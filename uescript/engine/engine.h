#pragma once
#include <uescript.h>
#include "objects.h"
#include "strings.h"

using ProcessEventFn = void (*)(UObject* object, UObject* function, void* params);
using DrawTransitionFn = void (*)(UObject* viewport_client, UObject* canvas);
using FreeMemoryFn = void (*)(void* memory);
using GetObjectNameFn = void (*)(FString* dest, const UObject* object);
using GetAllActorsOfClassFn = void (*)(UObject* world_context, UObject* klass, TArray* dest);
using StaticFindObjectFn = UObject *(*)(UObject*, UObject*, const wchar_t*, bool);
using WndProcFn = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);

using FNameToStringFn = FString *(*)(const FName* in, FString* dest);

using WorldToScreenFn = bool (*)(APlayerController* controller, Vector3 world_pos, Vector2* dst, bool unk);

using K2DrawTextFn = void (*)(
    UObject* canvas,
    void* font,
    const FString& text,
    Vector2 screenpos,
    Vector2 scale,
    FLinearColor color,
    float kerning,
    FLinearColor shadow_color,
    Vector2 shadow_offset,
    bool center_x,
    bool center_y,
    bool outlined,
    FLinearColor outline_color);

using K2DrawLineFn = void (*)(UObject* canvas, Vector2 from, Vector2 to, float thickness, FLinearColor color);
using K2DrawFilledRectFn = void (*)(UObject* hud, FLinearColor color, float x, float y, float w, float h);
using K2SizeOfTextFn = Vector2 *(*)(UObject* canvas, uint64_t* unk, void* font, const FString& text, Vector2 scale);

struct APawn;

using UEngine = uint64_t;
using TNameEntryArray = uint64_t;

struct EnginePointers
{
    ProcessEventFn ProcessEvent;
    DrawTransitionFn DrawTransition;
    FreeMemoryFn FreeMemory;
    GetObjectNameFn GetObjectName;
    StaticFindObjectFn StaticFindObject;
    GetAllActorsOfClassFn GetAllActorsOfClass;
    FNameToStringFn FNameToString;

    WorldToScreenFn WorldToScreen;
    K2DrawTextFn DrawText;
    K2DrawLineFn DrawLine;
    K2DrawFilledRectFn DrawFilledRect;
    K2SizeOfTextFn SizeOfText;

    UObjectArray* UObjectArray;
    UWorld** UWorld;
    UEngine** UEngine;
    TNameEntryArray** GNames;
};

inline EnginePointers g_EP;

class UE final
{
public:
    UE() = delete;

    static APlayerController* GetPlayerController();
    static UGameInstance* GetGameInstance();
    static APawn* GetLocalPawn();
    static UWorld* GetUWorld();
    static UEngine* GetUEngine();
    static TNameEntryArray* GetGNames();

    static bool ProcessEvent(UObject* object, UObject* function, void* params);
    static UEStr GetObjectName(const UObject* object);
    static UEStr FNameToString(const FName* name);
    static std::string GetAsciiObjectName(const UObject* object);

    template <int Size>
    constexpr static std::string_view GetAsciiObjectNameFast(const UObject* object, std::array<char, Size>& buffer)
    {
        const UEStr& wide_name = GetObjectName(object);
        return StringUtl::WideToAsciiStringFast(wide_name, buffer);
    }

    static UObject* StaticFindObject(const std::wstring_view& path);
    static UObject* StaticFindObject(const std::string_view& path);
    static TArray GetAllActorsOfClass(UObject* world_context, UObject* klass);
    static void FreeMemory(void* memory);
};
