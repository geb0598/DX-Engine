#pragma once

#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"   
#include "Vector.h"
#include "UEContainer.h"
#include"FireballComponent.h"
using namespace json;


// ========================================
// Version 1 (Legacy - 하위 호환용)
// ========================================
struct FPrimitiveData
{
    uint32 UUID = 0;
    FVector Location;
    FVector Rotation;
    FVector Scale;
    FString Type;
    FString ObjStaticMeshAsset;
};



struct FActorData
{
    uint32 UUID = 0;
    FString Type;  // "StaticMeshActor" 등
    FString Name;
    uint32 RootComponentUUID = 0;
};

struct FPerspectiveCameraData
{
    FVector Location;
	FVector Rotation;
	float FOV;
	float NearClip;
	float FarClip;
};

struct FSceneData
{
    uint32 Version = 2;
    uint32 NextUUID = 0;
    TArray<FActorData> Actors;
    TArray<FComponentData> Components;
    FPerspectiveCameraData Camera;
};

struct FProjectileMovementProperty
{
    float InitialSpeed = 1000.f;
    float MaxSpeed = 3000.f;
    float GravityScale = 1.0f;
};

struct FRotationMovementProperty
{
    FVector RotationRate = FVector(0, 0, 0);
    FVector PivotTranslation = FVector(0, 0, 0);
    bool bRotationInLocalSpace = false;
};

struct FLightProperty
{
    float Intensity = 1.0f;
    // Store RGBA as bytes (0-255) to match FColor usage
    uint8 R = 220;
    uint8 G = 220;
    uint8 B = 220;
    uint8 A = 255;
    bool bVisible = true;
};

struct FLocalLightProperty
{
    float AttenuationRadius = 10.0f;
};

struct FPointLightProperty
{
    float AttenuationRadius = 15.0f;
    float LightFalloffExponent = 1.0f;
};

struct FSpotLightProperty
{
    float InnerConeAngle = 10.0f;
    float OuterConeAngle = 20.0f;
};
struct FComponentData
{
    uint32 UUID = 0;
    uint32 OwnerActorUUID = 0;
    uint32 ParentComponentUUID = 0;  // 0이면 RootComponent (부모 없음)
    FString Type;  // "StaticMeshComponent", "AABoundingBoxComponent" 등

    // Transform
    FVector RelativeLocation;
    FVector RelativeRotation;
    FVector RelativeScale;

    // Type별 속성
    FString StaticMesh;  // StaticMeshComponent: Asset path
    TArray<FString> Materials;  // StaticMeshComponent: Materials
    FString TexturePath;  // DecalComponent, BillboardComponent: Texture path
    FFireBallProperty FireBallProperty; // FireballComponent
    FLightProperty LightProperty; // Light Components
    FLocalLightProperty LocalLightProperty; // LocalLightComponent
    FPointLightProperty PointLightProperty; // PointLightComponent / SpotLightComponent base
    FSpotLightProperty SpotLightProperty;   // SpotLightComponent specific
    // 신규
    FProjectileMovementProperty ProjectileMovementProperty;
    FRotationMovementProperty RotationMovementProperty;
};



class FSceneLoader
{
public:
    // Version 2 API
    static FSceneData LoadV2(const FString& FileName);
    static void SaveV2(const FSceneData& SceneData, const FString& SceneName);

    // Legacy Version 1 API (하위 호환)
    static TArray<FPrimitiveData> Load(const FString& FileName, FPerspectiveCameraData* OutCameraData);
    static void Save(TArray<FPrimitiveData> InPrimitiveData, const FPerspectiveCameraData* InCameraData, const FString& SceneName);

    static bool TryReadNextUUID(const FString& FilePath, uint32& OutNextUUID);

private:
    static TArray<FPrimitiveData> Parse(const JSON& Json);
    static FSceneData ParseV2(const JSON& Json);
};
