#pragma once

#include "Actor/Actor.h"
#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Component/Component.h"

class UScene
{
private:
    FString SceneName;
    TArray<AActor*> SceneActors;
    int32 SceneVersion;
    AActor* MainCameraActor;  // 메인 카메라 참조

public:
    UScene();
    UScene(const FString& Name);
    ~UScene();

    // Scene Management
    void AddActor(AActor* Actor);
    void RemoveActor(AActor* Actor);
    void ClearActors();
    
    // Getters
    const FString& GetSceneName() const { return SceneName; }
    void SetSceneName(const FString& Name) { SceneName = Name; }
    
    const TArray<AActor*>& GetActors() const { return SceneActors; }
    AActor* GetMainCameraActor() const { return MainCameraActor; }

    int32 GetVersion() const { return SceneVersion; }
    void SetVersion(int32 Version) { SceneVersion = Version; }
    
    // Scene Operations
    size_t GetActorCount() const { return SceneActors.size(); }
    AActor* GetActorByIndex(size_t Index) const;

private:
    // 카메라 액터를 생성하는 내부 함수
    void CreateMainCamera();
};