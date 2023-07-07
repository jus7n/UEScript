#pragma once
#include <cstdint>
#include "strings.h"

#define UNREAL_VERSION_421 421
#define UNREAL_VERSION UNREAL_VERSION_421

/*
 * Structs taken from ReClass and the Unreal Engine 4.21 source. Porting to a different engine version will probably
 * require these structs to be changed.
 *
 * TODO: Struct definitions should be defined from Lua for portability.
*/

template <typename T>
struct BasicTArray
{
    T* Data;
    int32_t Count;
    int32_t Max;
};

using TArray = BasicTArray<void*>;

struct Vector3
{
    float X;
    float Y;
    float Z;
};

struct Vector2
{
    float X;
    float Y;
};

struct FLinearColor
{
    float R;
    float G;
    float B;
    float A;

    constexpr static FLinearColor FromRGBA(const int r, const int g, const int b, const int a)
    {
        return {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    }
};

struct TSet
{
    void* Elements; // 0x0000
    int32_t Hash; // 0x0008
    int32_t HashSize; // 0x000C
}; // Size: 0x0010

#if UNREAL_VERSION == UNREAL_VERSION_421 // 4.21

struct UObject
{
    void* vtable; // 0x0000
    int32_t Flags; // 0x0008
    uint32_t InternalIndex; // 0x000C
    struct UObject* Class; // 0x0010
    struct FName Name; // 0x0018
    struct UObject* Outer; // 0x0020
}; // Size: 0x0028

struct UProperty
{
    UObject Super;
    struct UField* next;
    int32_t ArrayDim;
    int32_t ElementSize;
    char _pad[0x9];
    int32_t Offset_Internal;
};

struct FUObjectItem
{
    UObject* Object;
    // Internal flags
    int32_t Flags;
    // UObject Owner Cluster Index
    int32_t ClusterRootIndex;
    // Weak Object Pointer Serial number associated with the object
    int32_t SerialNumber;
    char _pad[0x4];
};

struct UObjectArray
{
    int32_t ObjFirstGCIndex; // 0x0000
    int32_t ObjLastNonGCIndex; // 0x0004
    int32_t MaxObjectsNotConsideredByGC; // 0x0008
    bool OpenForDisregardForGC; // 0x000C
    char pad_000D[3]; // 0x000D
    /** Master table to chunks of pointers **/
    FUObjectItem** Objects; // 0x0010
    void* PreAllocatedObjects; // 0x0018
    int32_t MaxElements; // 0x0020
    int32_t NumElements; // 0x0024
    int32_t MaxChunks; // 0x0028
    int32_t NumChunks; // 0x002C

    FUObjectItem* GetObject(int32_t Index) const
    {
        constexpr int NumElementsPerChunk = 64 * 1024;
        const int32_t ChunkIndex = Index / NumElementsPerChunk;
        const int32_t WithinChunkIndex = Index % NumElementsPerChunk;
        FUObjectItem* Chunk = Objects[ChunkIndex];
        return Chunk + WithinChunkIndex;
    }
};

struct APlayerController
{
    char pad_0000[0x3A8];
    struct UPlayer* Player;
    struct APawn* AcknowledgedPawn;
};

struct UPlayer
{
    void* vtable;
    UObject Super;
    APlayerController* PlayerController;
};

struct UGameInstance
{
    void* vtable; // 0x0000
    UObject Super; // 0x0008
    void* WorldContext; // 0x0030
    BasicTArray<UPlayer*> LocalPlayers; // 0x0038
    void* OnlineSession; // 0x0048
    TArray ReferencedObjects; // 0x0050
}; // Size: 0x0060

struct UWorld
{
    void* vtable; // 0x0000
    UObject Super; // 0x0008
    void* PersistentLevel; // 0x0030
    void* NetDriver; // 0x0038
    void* LineBatcher; // 0x0040
    void* PersistentLineBatcher; // 0x0048
    void* ForegroundLineBatcher; // 0x0050
    void* NetworkManager; // 0x0058
    void* PhysicsCollisionHandler; // 0x0060
    TArray ExtraReferencedObjects; // 0x0068
    TArray PerModuleDataObjects; // 0x0078
    TArray StreamingLevels; // 0x0088
    TSet StreamingLevelsToConsider; // 0x0098
    FString StreamingLevelsPrefix; // 0x00A8
    void* CurrentLevelPendingVisibility; // 0x00B8
    void* CurrentLevelPendingInvisibility; // 0x00C0
    void* DemoNetDriver; // 0x00C8
    void* MyParticleEventManager; // 0x00D0
    void* DefaultPhysicsVolume; // 0x00D8
    TArray ViewLocationsRenderedLastFrame; // 0x00E0
    char pad_00F0[152]; // 0x00F0
    void* CurrentLevel; // 0x0188
    UGameInstance* OwningGameInstance; // 0x0190
    TArray ParameterCollectionInstances; // 0x0198
    void* CanvasForRenderingToTarget; // 0x01A8
    void* CanvasForDrawMaterialToRenderTarget; // 0x01B0
    void* Scene; // 0x01B8
    int64_t FeatureLevel; // 0x01C0
    TArray ControllerList; // 0x01C8
    TArray PlayerControllerList; // 0x01D8
    TArray PawnList; // 0x01E8
    TArray AutoCameraActorList; // 0x01F8
    TArray NonDefaultPhysicsVolumeList; // 0x0208
    void* PhysicsScene; // 0x0218
    TSet ComponentsThatNeedEndOfFrameUpdate; // 0x0220
}; // Size: 0x0230
#else
#pragma error "Unsupported Unreal Engine version"
#endif
