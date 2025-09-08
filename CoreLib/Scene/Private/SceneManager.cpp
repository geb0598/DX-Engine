#include "../Public/SceneManager.h"
#include "Renderer/Renderer.h"
#include "Json/json.hpp"
#include <fstream>

// Static member initialization
USceneManager* USceneManager::Instance = nullptr;

USceneManager::USceneManager()
{
}

USceneManager::~USceneManager()
{
}

USceneManager& USceneManager::GetInstance()
{
    if (Instance == nullptr)
    {
        Instance = new USceneManager();
    }
    return *Instance;
}

void USceneManager::DestroyInstance()
{
    if (Instance)
    {
        delete Instance;
        Instance = nullptr;
    }
}

void USceneManager::NewScene(const FString& SceneName)
{
    CurrentScene.reset();

    // 새로운 씬 생성 (이미 카메라가 포함됨)
    CurrentScene = std::make_unique<UScene>(SceneName);
}

void USceneManager::SaveScene(const FString& FilePath)
{
    if (!CurrentScene)
    {
        return;
    }
    
    json::JSON Obj = json::Object();
    Obj["Version"] = CurrentScene->GetVersion();
    Obj["SceneName"] = CurrentScene->GetSceneName();
    Obj["NextUUID"] = static_cast<int32>(UEngineStatics::NextUUID);
    Obj["Primitives"] = json::Object();
    
    // 현재 씬의 모든 액터 저장 (카메라 제외)
    const TArray<AActor*>& SceneActors = CurrentScene->GetActors();
    int primitiveIndex = 0;
    
    for (size_t i = 0; i < SceneActors.size(); ++i)
    {
        AActor* Actor = SceneActors[i];
        if (Actor && Actor != CurrentScene->GetMainCameraActor())  // 카메라는 저장하지 않음
        {
            auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
            auto SceneComponent = Actor->GetComponent<USceneComponent>();
            
            if (PrimitiveComponent && SceneComponent)
            {
                FVector Location = SceneComponent->GetLocation();
                FVector Rotation = SceneComponent->GetRotation();
                FVector Scale = SceneComponent->GetScale();
                EPrimitiveType Type = PrimitiveComponent->GetPrimitiveType();
                
                SavePrimitive(Obj, primitiveIndex++, Location, Rotation, Scale, Type);
            }
        }
    }
    
    std::ofstream file(FilePath.c_str());
    if (!file.is_open())
    {
        return;
    }
    
    file << Obj.dump(1);
    file.close();
}

void USceneManager::LoadScene(const FString& FilePath)
{
    std::ifstream file(FilePath.c_str());
    if (!file.is_open())
    {
        return;
    }
    
    // 새로운 씬 생성 (카메라 포함)
    NewScene("Loaded Scene");
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    json::JSON Obj = json::JSON::Load(content);
    
    // 씬 정보 로드
    if (Obj.hasKey("SceneName"))
    {
        CurrentScene->SetSceneName(Obj["SceneName"].ToString());
    }
    
    if (Obj.hasKey("Version"))
    {
        CurrentScene->SetVersion(Obj["Version"].ToInt());
    }
    
    // 프리미티브들 로드
    if (Obj.hasKey("Primitives"))
    {
        json::JSON Primitives = Obj["Primitives"];
        for (const auto& p : Primitives.ObjectRange())
        {
            json::JSON Primitive = p.second;
            
            FVector Location = FVector(
                static_cast<float>(Primitive["Location"][0].ToFloat()),
                static_cast<float>(Primitive["Location"][1].ToFloat()),
                static_cast<float>(Primitive["Location"][2].ToFloat())
            );
            FVector Rotation = FVector(
                static_cast<float>(Primitive["Rotation"][0].ToFloat()),
                static_cast<float>(Primitive["Rotation"][1].ToFloat()),
                static_cast<float>(Primitive["Rotation"][2].ToFloat())
            );
            FVector Scale = FVector(
                static_cast<float>(Primitive["Scale"][0].ToFloat()),
                static_cast<float>(Primitive["Scale"][1].ToFloat()),
                static_cast<float>(Primitive["Scale"][2].ToFloat())
            );
            
            FString PrimitiveType = Primitive["Type"].ToString();
            EPrimitiveType Type = StringToPrimitiveType(PrimitiveType);
            
            AActor* LoadedActor = CreateActorFromPrimitive(Location, Rotation, Scale, Type);
            if (LoadedActor)
            {
                CurrentScene->AddActor(LoadedActor);
            }
        }
    }
    
    if (Obj.hasKey("NextUUID"))
    {
        int32 NextUUID = Obj["NextUUID"].ToInt();
        UEngineStatics::NextUUID = NextUUID;
    }
    
    file.close();
}

void USceneManager::AddActorToCurrentScene(AActor* Actor)
{
    if (CurrentScene && Actor)
    {
        CurrentScene->AddActor(Actor);
    }
}

void USceneManager::RemoveActorFromCurrentScene(AActor* Actor)
{
    if (CurrentScene && Actor)
    {
        CurrentScene->RemoveActor(Actor);
    }
}

AActor* USceneManager::GetMainCameraActor() const
{
    if (CurrentScene)
    {
        return CurrentScene->GetMainCameraActor();
    }
    return nullptr;
}

// Helper functions
void USceneManager::SavePrimitive(json::JSON& Obj, int Index, const FVector& Location, const FVector& Rotation, const FVector& Scale, EPrimitiveType Type)
{
    FString IndexStr = std::to_string(Index);
    Obj["Primitives"][IndexStr] = json::Object();
    Obj["Primitives"][IndexStr]["Location"] = json::Array(Location.X, Location.Y, Location.Z);
    Obj["Primitives"][IndexStr]["Rotation"] = json::Array(Rotation.X, Rotation.Y, Rotation.Z);
    Obj["Primitives"][IndexStr]["Scale"] = json::Array(Scale.X, Scale.Y, Scale.Z);
    Obj["Primitives"][IndexStr]["Type"] = PrimitiveTypeToString(Type);
}

AActor* USceneManager::CreateActorFromPrimitive(const FVector& Location, const FVector& Rotation, const FVector& Scale, EPrimitiveType Type)
{
    URenderer& Renderer = URenderer::GetInstance();
    
    AActor* NewActor = new AActor();
    
    TArray<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc(
        std::begin(FVertex::InputLayoutDesc), 
        std::end(FVertex::InputLayoutDesc)
    );
    
    std::shared_ptr<UVertexShader> VertexShader = std::make_shared<UVertexShader>(
        Renderer.GetDevice(),
        "./Shader/VertexShader.hlsl", 
        "main",
        InputLayoutDesc
    );
    
    std::shared_ptr<UPixelShader> PixelShader = std::make_shared<UPixelShader>(
        Renderer.GetDevice(),
        "./Shader/PixelShader.hlsl",
        "main"
    );
    
    NewActor->AddComponent<UPrimitiveComponent>(NewActor, Type, VertexShader, PixelShader);
    NewActor->AddComponent<USceneComponent>(NewActor, Location, Rotation, Scale);
    
    return NewActor;
}

FString USceneManager::PrimitiveTypeToString(EPrimitiveType Type)
{
    switch (Type)
    {
    case EPrimitiveType::EPT_Triangle:
        return "Triangle";
    case EPrimitiveType::EPT_Cube:
        return "Cube";
    case EPrimitiveType::EPT_Sphere:
        return "Sphere";
    default:
        return "Triangle";
    }
}

EPrimitiveType USceneManager::StringToPrimitiveType(const FString& TypeStr)
{
    if (TypeStr == "Triangle")
        return EPrimitiveType::EPT_Triangle;
    else if (TypeStr == "Cube")
        return EPrimitiveType::EPT_Cube;
    else if (TypeStr == "Sphere")
        return EPrimitiveType::EPT_Sphere;
    else
        return EPrimitiveType::EPT_Triangle;
}