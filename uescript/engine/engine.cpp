#include <uescript.h>
#include "engine.h"

APlayerController* UE::GetPlayerController()
{
    const UWorld* world = *g_EP.UWorld;
    if (!world)
        return nullptr;

    const UGameInstance* game = world->OwningGameInstance;
    if (!game)
        return nullptr;

    const UPlayer* local_player = game->LocalPlayers.Data[0];
    if (!local_player)
        return nullptr;

    return local_player->PlayerController;
}

UGameInstance* UE::GetGameInstance()
{
    const UWorld* world = *g_EP.UWorld;
    if (!world)
        return nullptr;

    UGameInstance* game = world->OwningGameInstance;
    if (!game)
        return nullptr;

    return game;
}

APawn* UE::GetLocalPawn()
{
    const APlayerController* controller = GetPlayerController();
    if (!controller)
        return nullptr;

    return controller->AcknowledgedPawn;
}

UWorld* UE::GetUWorld()
{
    return *g_EP.UWorld;
}

UEngine* UE::GetUEngine()
{
    return *g_EP.UEngine;
}

TNameEntryArray* UE::GetGNames()
{
    return *g_EP.GNames;
}

bool UE::ProcessEvent(UObject* object, UObject* function, void* params)
{
    using Fn = void (*)(UObject*, UObject*, void*);
    const int32_t old_flags = function->Flags;
#ifdef SCRIPT_SAFETY_ON
    __try
    {
#endif
        function->Flags |= 0x00000400;
        g_EP.ProcessEvent(object, function, params);
        function->Flags = old_flags;
        return true;
#ifdef SCRIPT_SAFETY_ON
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#endif
}

UEStr UE::GetObjectName(const UObject* object)
{
    FString dest{};
    g_EP.GetObjectName(&dest, object);
    return UEStr(dest);
}

UEStr UE::FNameToString(const FName* name)
{
    FString dest{};
    g_EP.FNameToString(name, &dest);
    return UEStr(dest);
}

std::string UE::GetAsciiObjectName(const UObject* object)
{
    const UEStr& wide_name = GetObjectName(object);
    return StringUtl::WideToAsciiString(wide_name);
}

UObject* UE::StaticFindObject(const std::wstring_view& path)
{
    return g_EP.StaticFindObject(nullptr, reinterpret_cast<UObject*>(-1), path.data(), false);
}

UObject* UE::StaticFindObject(const std::string_view& path)
{
    const std::wstring& wide_path = StringUtl::AsciiToWideString(path);
    return StaticFindObject(wide_path);
}

TArray UE::GetAllActorsOfClass(UObject* world_context, UObject* klass)
{
    TArray result = {};
    g_EP.GetAllActorsOfClass(world_context, klass, &result);
    return result;
}

void UE::FreeMemory(void* memory)
{
    g_EP.FreeMemory(memory);
}
