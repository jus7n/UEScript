#include <uescript.h>
#include "unreal_sdk.h"

#include <engine/engine.h>
#include <lua/lua_engine.h>

void UnrealSDK::InitInternal(lua_State* L)
{
    using namespace sdk::util;

    lua_newtable(L);
    {
        lua_pushcfunction(L, WrapPtr<UE::GetUEngine>);
        lua_setfield(L, -2, "GetEngine");

        lua_pushcfunction(L, WrapPtr<UE::GetGNames>);
        lua_setfield(L, -2, "GetGNames");

        lua_pushcfunction(L, WrapPtr<UE::GetUWorld>);
        lua_setfield(L, -2, "GetUWorld");
        {
            lua_pushcfunction(L, (WrapField<UWorld, &UWorld::PersistentLevel>));
            lua_setfield(L, -2, "GetWorldPersistentLevel");

            lua_pushcfunction(L, (WrapField<UWorld, &UWorld::NetDriver>));
            lua_setfield(L, -2, "GetWorldNetDriver");

            lua_pushcfunction(L, (WrapField<UWorld, &UWorld::NetworkManager>));
            lua_setfield(L, -2, "GetWorldNetworkManager");

            lua_pushcfunction(L, (WrapField<UWorld, &UWorld::CurrentLevel>));
            lua_setfield(L, -2, "GetWorldCurrentLevel");
        }

        lua_pushcfunction(L, WrapPtr<UE::GetLocalPawn>);
        lua_setfield(L, -2, "GetLocalPawn");

        lua_pushcfunction(L, WrapPtr<UE::GetPlayerController>);
        lua_setfield(L, -2, "GetPlayerController");
        {
            lua_pushcfunction(L, (WrapField<APlayerController, &APlayerController::Player>));
            lua_setfield(L, -2, "GetPlayerControllerPlayer");

            lua_pushcfunction(L, (WrapField<APlayerController, &APlayerController::AcknowledgedPawn>));
            lua_setfield(L, -2, "GetPlayerControllerAcknowledgedPawn");
        }

        lua_pushcfunction(L, WrapPtr<UE::GetGameInstance>);
        lua_setfield(L, -2, "GetGameInstance");
        {
            lua_pushcfunction(L, (WrapField<UGameInstance, &UGameInstance::WorldContext>));
            lua_setfield(L, -2, "GetGameInstanceWorldContext");

            lua_pushcfunction(L, (WrapField<UGameInstance, &UGameInstance::OnlineSession>));
            lua_setfield(L, -2, "GetGameInstanceOnlineSession");
        }

        lua_pushcfunction(L, UnrealSDK::WorldToScreen);
        lua_setfield(L, -2, "WorldToScreen");
        lua_pushcfunction(L, UnrealSDK::DrawText);
        lua_setfield(L, -2, "DrawText");
        lua_pushcfunction(L, UnrealSDK::DrawLine);
        lua_setfield(L, -2, "DrawLine");
        lua_pushcfunction(L, UnrealSDK::DrawRect);
        lua_setfield(L, -2, "DrawRect");
        lua_pushcfunction(L, UnrealSDK::DrawFilledRect);
        lua_setfield(L, -2, "DrawFilledRect");
        lua_pushcfunction(L, UnrealSDK::SizeOfText);
        lua_setfield(L, -2, "SizeOfText");

        lua_pushcfunction(L, UnrealSDK::FindObjectSlow);
        lua_setfield(L, -2, "FindObjectSlow");

        lua_pushcfunction(L, UnrealSDK::FindAllObjectsSlow);
        lua_setfield(L, -2, "FindAllObjectsSlow");

        lua_pushcfunction(L, UnrealSDK::StaticFindObject);
        lua_setfield(L, -2, "StaticFindObject");
        {
            lua_pushcfunction(L, UnrealSDK::GetObjectName);
            lua_setfield(L, -2, "GetObjectName");

            lua_pushcfunction(L, (WrapField<UObject, &UObject::Class>));
            lua_setfield(L, -2, "GetObjectClass");

            lua_pushcfunction(L, (WrapField<UObject, &UObject::Outer>));
            lua_setfield(L, -2, "GetObjectOuter");

            lua_pushcfunction(L, (WrapField<UObject, &UObject::InternalIndex>));
            lua_setfield(L, -2, "GetObjectInternalIndex");
        }

        lua_pushcfunction(L, UnrealSDK::GetAllActorsOfClass);
        lua_setfield(L, -2, "GetAllActorsOfClass");

        lua_pushcfunction(L, UnrealSDK::ProcessEvent);
        lua_setfield(L, -2, "ProcessEvent");

        lua_pushcfunction(L, UnrealSDK::FNameToString);
        lua_setfield(L, -2, "FNameToString");

        lua_pushcfunction(L, [](lua_State *L)
                          { UnrealSDK::GenerateUnrealTypes(L); return 0; });
        lua_setfield(L, -2, "RegenerateTypes");
    }
    lua_setglobal(L, "unreal");

    GenerateUnrealTypes(L);
}

void UnrealSDK::GenerateUnrealTypes(lua_State* L)
{
    const auto& then = chrono::system_clock::now();

    const StackGuard outer_guard(L);

    lua_getglobal(L, "unreal");
    lua_getfield(L, -1, "Types");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
    }

    std::array<char, 512> name_buf{};
    std::array<char, 512> outer_name_buf{};
    std::array<char, 512> class_name_buf{};

    const UObjectArray* array = g_EP.UObjectArray;
    for (int32_t index = 0; index < array->NumElements; index++)
    {
        const FUObjectItem* item = array->GetObject(index);
        if (item == nullptr || item->Object == nullptr)
            continue;

        UObject* object = item->Object;
        if (object == nullptr || object->Outer == nullptr || object->Class == nullptr)
            continue;

        const std::string_view& class_name = UE::GetAsciiObjectNameFast(object->Class, class_name_buf);

        const bool is_function = class_name.find("Function") != std::string::npos;
        bool is_property = class_name.find("Property") != std::string::npos;

        // Kind of a gross solution but it's not that slow and it works.
        if (!is_property)
        {
            const UObject* cur = object;
            while (cur && cur != cur->Class)
            {
                const UEStr& w_name = UE::GetObjectName(cur);
                if (w_name.view().find(L"Property") != std::wstring_view::npos)
                {
                    is_property = true;
                    break;
                }

                cur = cur->Class;
            }
        }

        if (!is_function && !is_property)
            continue;

        const std::string_view& outer_name = UE::GetAsciiObjectNameFast(object->Outer, outer_name_buf);
        const std::string_view& name = UE::GetAsciiObjectNameFast(object, name_buf);

        const StackGuard guard(L);

        lua_getfield(L, -1, outer_name.data());

        // Does outer_name not exist in the table? Create it and push it onto the stack.
        if (lua_type(L, -1) == LUA_TNIL)
        {
            lua_pop(L, 1);
            lua_newtable(L);
            // We need the table to still be on the stack
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, outer_name.data());
        }

        // Skip if there's already an object in the table.
        {
            lua_getfield(L, -1, name.data());
            const bool already_exists = lua_type(L, -1) != LUA_TNIL;
            lua_pop(L, 1);
            if (already_exists)
                continue;
        }

        if (is_function)
        {
            lua_newtable(L);
            {
                lua_pushinteger(L, reinterpret_cast<lua_Integer>(object));
                lua_setfield(L, -2, "offset");

                lua_pushinteger(L, 0);
                lua_setfield(L, -2, "size");

                lua_pushboolean(L, true);
                lua_setfield(L, -2, "is_function");
            }
            lua_setfield(L, -2, name.data());
            continue;
        }

        const UProperty* prop = reinterpret_cast<UProperty*>(object);

        if (prop->ElementSize == 0)
            continue;

        lua_newtable(L);
        {
            lua_pushinteger(L, prop->Offset_Internal);
            lua_setfield(L, -2, "offset");

            lua_pushinteger(L, prop->ElementSize);
            lua_setfield(L, -2, "size");

            lua_pushboolean(L, false);
            lua_setfield(L, -2, "is_function");
        }
        lua_setfield(L, -2, name.data());
    }

    lua_setfield(L, -2, "Types");

    const auto& delta = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - then);
    std::cout << "Generated Unreal types in " << delta.count() << " ms" << std::endl;
}

int UnrealSDK::WorldToScreen(lua_State* L)
{
    APlayerController* controller = UE::GetPlayerController();
    TCheckPtrHot(controller);

    const float wx = static_cast<float>(luaL_checknumber(L, -3));
    const float wy = static_cast<float>(luaL_checknumber(L, -2));
    const float wz = static_cast<float>(luaL_checknumber(L, -1));

    Vector2 out{};
    if (const bool visible = g_EP.WorldToScreen(controller, Vector3{wx, wy, wz}, &out, false); !visible)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    lua_pushnumber(L, out.X);
    lua_pushnumber(L, out.Y);
    return 3;
}

int UnrealSDK::DrawText(lua_State* L)
{
    // Defaults, maybe DrawTextEx?
    constexpr Vector2 scale = {1.f, 1.f};
    constexpr float kerning = 1.f;
    constexpr FLinearColor shadow_color = FLinearColor::FromRGBA(0, 0, 0, 255);
    constexpr Vector2 shadow_offset = {1.f, 1.f};
    constexpr bool center_x = false;
    constexpr bool center_y = false;
    constexpr FLinearColor outline_color = shadow_color;

    const auto canvas = reinterpret_cast<UObject*>(luaL_checkinteger(L, -10));
    TCheckPtrHot(canvas);

    const auto font = reinterpret_cast<void*>(luaL_checkinteger(L, -9));
    const char* text = luaL_checkstring(L, -8);

    const float x = static_cast<float>(luaL_checknumber(L, -7));
    const float y = static_cast<float>(luaL_checknumber(L, -6));

    const int r = static_cast<int>(luaL_checknumber(L, -5));
    const int g = static_cast<int>(luaL_checknumber(L, -4));
    const int b = static_cast<int>(luaL_checknumber(L, -3));
    const int a = static_cast<int>(luaL_checknumber(L, -2));

    luaL_checktype(L, -1, LUA_TBOOLEAN);
    const bool outlined = lua_toboolean(L, -1);

    const std::wstring_view& w_text = StringUtl::AsciiToWideStringFast(text, conv_buf::g_WideBuf);

    const FString str{
        w_text.data(), static_cast<int32_t>(w_text.size() + 1), static_cast<int32_t>(w_text.size() + 1)
    };

    g_EP.DrawText(canvas,
                  font,
                  str,
                  Vector2{x, y},
                  scale,
                  FLinearColor::FromRGBA(r, g, b, a),
                  kerning,
                  shadow_color,
                  shadow_offset,
                  center_x,
                  center_y,
                  outlined,
                  outline_color);

    return 0;
}

int UnrealSDK::DrawLine(lua_State* L)
{
    const auto canvas = reinterpret_cast<UObject*>(luaL_checkinteger(L, -10));
    const float x = static_cast<float>(luaL_checknumber(L, -9));
    const float y = static_cast<float>(luaL_checknumber(L, -8));
    const float x2 = static_cast<float>(luaL_checknumber(L, -7));
    const float y2 = static_cast<float>(luaL_checknumber(L, -6));
    const float thickness = static_cast<float>(luaL_checknumber(L, -5));
    const int r = static_cast<int>(luaL_checknumber(L, -4));
    const int g = static_cast<int>(luaL_checknumber(L, -3));
    const int b = static_cast<int>(luaL_checknumber(L, -2));
    const int a = static_cast<int>(luaL_checknumber(L, -1));

    g_EP.DrawLine(canvas, Vector2{x, y}, Vector2{x2, y2}, thickness, FLinearColor::FromRGBA(r, g, b, a));
    return 0;
}

int UnrealSDK::DrawRect(lua_State* L)
{
    // Defaults.
    constexpr float thickness = 1.f;

    const auto canvas = reinterpret_cast<UObject*>(luaL_checkinteger(L, -9));
    const float x = static_cast<float>(luaL_checknumber(L, -8));
    const float y = static_cast<float>(luaL_checknumber(L, -7));
    const float size_x = static_cast<float>(luaL_checknumber(L, -6));
    const float size_y = static_cast<float>(luaL_checknumber(L, -5));
    const int r = static_cast<int>(luaL_checknumber(L, -4));
    const int g = static_cast<int>(luaL_checknumber(L, -3));
    const int b = static_cast<int>(luaL_checknumber(L, -2));
    const int a = static_cast<int>(luaL_checknumber(L, -1));

    const Vector2 pos{x, y};
    const Vector2 size{size_x, size_y};
    const auto color = FLinearColor::FromRGBA(r, g, b, a);

    g_EP.DrawLine(canvas, pos, Vector2{pos.X + size.X, pos.Y}, thickness, color);
    g_EP.DrawLine(canvas, pos, Vector2{pos.X, pos.Y + size.Y}, thickness, color);
    g_EP.DrawLine(canvas, Vector2{pos.X, pos.Y + size.Y}, Vector2{pos.X + size.X, pos.Y + size.Y}, thickness, color);
    g_EP.DrawLine(canvas, Vector2{pos.X + size.X, pos.Y}, Vector2{pos.X + size.X, pos.Y + size.Y}, thickness, color);

    return 0;
}

int UnrealSDK::DrawFilledRect(lua_State* L)
{
    const auto canvas = reinterpret_cast<UObject*>(luaL_checkinteger(L, -10));
    TCheckPtrHot(canvas);

    const auto hud = reinterpret_cast<UObject*>(luaL_checkinteger(L, -9));
    TCheckPtrHot(hud);

    const float x = static_cast<float>(luaL_checknumber(L, -8));
    const float y = static_cast<float>(luaL_checknumber(L, -7));
    const float w = static_cast<float>(luaL_checknumber(L, -6));
    const float h = static_cast<float>(luaL_checknumber(L, -5));
    const int r = static_cast<int>(luaL_checknumber(L, -4));
    const int g = static_cast<int>(luaL_checknumber(L, -3));
    const int b = static_cast<int>(luaL_checknumber(L, -2));
    const int a = static_cast<int>(luaL_checknumber(L, -1));

    // FIXME
    const auto hud_canvas = reinterpret_cast<UObject**>(reinterpret_cast<uint64_t>(hud) + 0x378);
    UObject* old_canvas = *hud_canvas;

    *hud_canvas = canvas;
    g_EP.DrawFilledRect(hud, FLinearColor::FromRGBA(r, g, b, a), x, y, w, h);
    *hud_canvas = old_canvas;
    return 0;
}

int UnrealSDK::SizeOfText(lua_State* L)
{
    constexpr Vector2 scale = {1.f, 1.f};

    const auto canvas = reinterpret_cast<UObject*>(luaL_checkinteger(L, -3));
    TCheckPtrHot(canvas);

    const auto font = reinterpret_cast<void*>(luaL_checkinteger(L, -2));
    const char* text = luaL_checkstring(L, -1);

    const std::wstring_view w_text = StringUtl::AsciiToWideStringFast(text, conv_buf::g_WideBuf);
    const FString str{w_text.data(), static_cast<int32_t>(w_text.size() + 1), static_cast<int32_t>(w_text.size() + 1)};

    uint64_t unk;
    const auto text_size = g_EP.SizeOfText(canvas, &unk, font, str, scale);

    lua_pushnumber(L, static_cast<lua_Number>(text_size->X));
    lua_pushnumber(L, static_cast<lua_Number>(text_size->Y));
    return 2;
}

int UnrealSDK::StaticFindObject(lua_State* L)
{
    const char* path = luaL_checkstring(L, -1);
    UObject* object = UE::StaticFindObject(path);

    object != nullptr ? lua_pushinteger(L, reinterpret_cast<lua_Integer>(object)) : lua_pushnil(L);

    return 1;
}

int UnrealSDK::FindObjectSlow(lua_State* L)
{
    const char* search = luaL_checkstring(L, -1);

    const UObjectArray* array = g_EP.UObjectArray;
    for (int32_t index = 0; index < array->NumElements; index++)
    {
        const FUObjectItem* item = array->GetObject(index);
        if (item == nullptr || item->Object == nullptr)
            continue;

        UObject* object = item->Object;
        if (object == nullptr)
            continue;

        const std::string_view& name = UE::GetAsciiObjectNameFast(object, conv_buf::g_AsciiBuf);
        if (name.find(search) != std::string::npos)
        {
            lua_pushinteger(L, reinterpret_cast<lua_Integer>(object));
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

int UnrealSDK::FindAllObjectsSlow(lua_State* L)
{
    StringUtl::AsciiToWideStringFast(luaL_checkstring(L, -1), conv_buf::g_WideBuf);

    lua_newtable(L);
    int i = 1;

    const UObjectArray* array = g_EP.UObjectArray;
    for (int32_t index = 0; index < array->NumElements; index++)
    {
        const FUObjectItem* item = array->GetObject(index);
        if (item == nullptr || item->Object == nullptr)
            continue;

        UObject* object = item->Object;
        if (object == nullptr)
            continue;

        const UEStr& object_name = UE::GetObjectName(object);
        if (object_name.view().find(conv_buf::g_WideBuf.data()) != std::wstring::npos)
        {
            lua_pushnumber(L, i);
            lua_pushinteger(L, reinterpret_cast<lua_Integer>(object));
            lua_settable(L, -3);
            i++;
        }
    }

    return 1;
}

int UnrealSDK::GetAllActorsOfClass(lua_State* L)
{
    const char* class_path = luaL_checkstring(L, -1);
    UObject* klass = UE::StaticFindObject(class_path);
    if (!klass)
    {
        lua_pushnil(L);
        return 1;
    }

    APawn* pawn = UE::GetPlayerController()->AcknowledgedPawn;
    if (!pawn)
    {
        lua_pushnil(L);
        return 1;
    }

    const auto& [Actors, Count, Max]
        = UE::GetAllActorsOfClass(reinterpret_cast<UObject*>(pawn), klass);

    lua_newtable(L);
    for (int i = 0; i < Count; i++)
    {
        if (Actors[i] == nullptr)
            continue;
        lua_pushnumber(L, i + 1);
        lua_pushinteger(L, reinterpret_cast<lua_Integer>(Actors[i]));
        lua_settable(L, -3);
    }

    return 1;
}

int UnrealSDK::FNameToString(lua_State* L)
{
    const FName* name = reinterpret_cast<FName*>(luaL_checkinteger(L, -1));
    TCheckPtrHot(name);

    const UEStr& string = UE::FNameToString(name);
    const std::string_view& cvt_name = StringUtl::WideToAsciiStringFast(string, conv_buf::g_AsciiBuf);
    lua_pushstring(L, cvt_name.data());
    return 1;
}

int UnrealSDK::ProcessEvent(lua_State* L)
{
    const auto object = reinterpret_cast<UObject*>(luaL_checkinteger(L, -3));
    TCheckPtrHot(object);

    const auto function = reinterpret_cast<UObject*>(luaL_checkinteger(L, -2));
    TCheckPtrHot(function);

    // Params can be null.
    if (const auto params = reinterpret_cast<void*>(luaL_checkinteger(L, -1)); !UE::ProcessEvent(
        object, function, params))
    {
        PrintCallstack(L);
        return luaL_error(L, "bad process event call");
    }
    return 0;
}

int UnrealSDK::GetObjectName(lua_State* L)
{
    const UObject* object = reinterpret_cast<UObject*>(luaL_checkinteger(L, -1));
    TCheckPtrHot(object);

    const std::string_view& name = UE::GetAsciiObjectNameFast(object, conv_buf::g_AsciiBuf);
    lua_pushstring(L, name.data());
    return 1;
}
