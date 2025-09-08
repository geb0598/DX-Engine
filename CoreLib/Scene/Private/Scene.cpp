#include "../Public/Scene.h"

UScene::UScene() 
    : SceneName("Untitled Scene"), SceneVersion(1), MainCameraActor(nullptr)
{
    CreateMainCamera();
}

UScene::UScene(const FString& Name) 
    : SceneName(Name), SceneVersion(1), MainCameraActor(nullptr)
{
    CreateMainCamera();
}

UScene::~UScene()
{
    ClearActors();

    GUDObjectArray.clear();
    GUDObjectFreeIndexArray.clear();

    UEngineStatics::NextUUID = 0;
}

void UScene::CreateMainCamera()
{
    // 메인 카메라 액터 생성
    MainCameraActor = new AActor();
    MainCameraActor->AddComponent<USceneComponent>(MainCameraActor, 
        FVector(0.0f, 0.0f, -30.0f),  // 위치
        FVector(0.0f, 0.0f, 0.0f),    // 회전
        FVector(1.0f, 1.0f, 1.0f)     // 스케일
    );
    MainCameraActor->AddComponent<UCameraComponent>(MainCameraActor, PIDIV2, 0.1f, 1000.0f);
    // 인풋 컴포넌트 추가해야함
    MainCameraActor->AddComponent<UInputComponent>(MainCameraActor);
    
    // 씬에 추가
    SceneActors.push_back(MainCameraActor);
}

void UScene::AddActor(AActor* Actor)
{
    if (Actor && std::find(SceneActors.begin(), SceneActors.end(), Actor) == SceneActors.end())
    {
        SceneActors.push_back(Actor);
    }
}

void UScene::RemoveActor(AActor* Actor)
{
    if (Actor == MainCameraActor)
    {
        // 메인 카메라는 제거할 수 없음
        return;
    }
    
    auto it = std::find(SceneActors.begin(), SceneActors.end(), Actor);
    if (it != SceneActors.end())
    {
        SceneActors.erase(it);
        delete Actor;  // 메모리 해제
    }
}

void UScene::ClearActors()
{
    for (AActor* Actor : SceneActors)
    {
        if (Actor)
        {
            delete Actor;
        }
    }
    SceneActors.clear();
    MainCameraActor = nullptr;  // 카메라 참조도 초기화
}

AActor* UScene::GetActorByIndex(size_t Index) const
{
    if (Index < SceneActors.size())
    {
        return SceneActors[Index];
    }
    return nullptr;
}