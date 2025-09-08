#pragma once

#include "Scene.h"
#include "Actor/Actor.h"
#include "Component/Component.h"
#include "Containers/Containers.h"
#include "Types/Types.h"
#include <memory>

namespace json { class JSON; }

class USceneManager
{
private:
    static USceneManager* Instance;
    std::unique_ptr<UScene> CurrentScene;
    
    USceneManager();
    ~USceneManager();
    
public:
    // Singleton Access
    static USceneManager& GetInstance();
    static void DestroyInstance();
    
    // Scene Management
    void NewScene(const FString& SceneName = "New Scene");
    void SaveScene(const FString& FilePath);
    void LoadScene(const FString& FilePath);
    
    // Current Scene Access
    UScene* GetCurrentScene() const { return CurrentScene.get(); }
    bool HasCurrentScene() const { return CurrentScene != nullptr; }
    
    // Scene Operations
    void AddActorToCurrentScene(AActor* Actor);
    void RemoveActorFromCurrentScene(AActor* Actor);
    
    // Camera Access
    AActor* GetMainCameraActor() const;
    
private:
    // Helper functions for serialization
    void SavePrimitive(json::JSON& Obj, int Index, const FVector& Location, const FVector& Rotation, const FVector& Scale, EPrimitiveType Type);
    AActor* CreateActorFromPrimitive(const FVector& Location, const FVector& Rotation, const FVector& Scale, EPrimitiveType Type);
    FString PrimitiveTypeToString(EPrimitiveType Type);
    EPrimitiveType StringToPrimitiveType(const FString& TypeStr);
};