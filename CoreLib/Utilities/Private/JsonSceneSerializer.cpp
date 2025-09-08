#include "../Public/JsonSceneSerializer.h"
#include "Renderer/Renderer.h"
#include "Json/json.hpp"

FString PrimitiveTypeToString(EPrimitiveType Type)
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

EPrimitiveType StringToPrimitiveType(const FString& TypeStr)
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

void SavePrimitive(json::JSON& Obj, int Index, FVector Location, FVector Rotation, FVector Scale, EPrimitiveType Type)
{
	FString IndexStr = std::to_string(Index);
	Obj["Primitives"][IndexStr] = json::Object();
	Obj["Primitives"][IndexStr]["Location"] = json::Array(Location.X, Location.Y, Location.Z);
	Obj["Primitives"][IndexStr]["Rotation"] = json::Array(Rotation.X, Rotation.Y, Rotation.Z);
	Obj["Primitives"][IndexStr]["Scale"] = json::Array(Scale.X, Scale.Y, Scale.Z);
	Obj["Primitives"][IndexStr]["Type"] = PrimitiveTypeToString(Type);
}

void NewScene()
{
	// 현재 씬의 모든 오브젝트 제거
	for (int i = static_cast<int>(GUDObjectArray.size()) - 1; i >= 0; --i)
	{
		if (GUDObjectArray[i] != nullptr)
		{
			// Actor 타입인지 확인하고 삭제
			AActor* Actor = static_cast<AActor*>(GUDObjectArray[i]);
			if (Actor)
			{
				delete Actor;  // 소멸자에서 자동으로 GUDObjectArray에서 제거
			}
		}
	}

	// 배열 정리 (nullptr 제거)
	GUDObjectArray.clear();
	GUDObjectFreeIndexArray.clear();

	// UUID 카운터 초기화
	UEngineStatics::NextUUID = 0;
}

// 미완성 함수
void SaveScene(const FString& FilePath, int32 Version)
{
	json::JSON Obj = json::Object();
	Obj["Version"] = Version;
	Obj["NextUUID"] = GUDObjectArray.size(); // 0번은 카메라
	Obj["Primitives"] = json::Object();

	for (size_t i = 0; i < GUDObjectArray.size(); ++i)
	{
		auto Actor = static_cast<AActor*>(GUDObjectArray[i]);
		if (Actor)
		{
			auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
			auto SceneComponent = Actor->GetComponent<USceneComponent>();
			
			if (PrimitiveComponent && SceneComponent)
			{
				FVector Location = SceneComponent->GetLocation();
				FVector Rotation = SceneComponent->GetRotation();
				FVector Scale = SceneComponent->GetScale();
				EPrimitiveType Type = PrimitiveComponent->GetPrimitiveType();
				
				SavePrimitive(Obj, static_cast<int>(i), Location, Rotation, Scale, Type);
			}
		}
	}

	std::ofstream file(FilePath.c_str());
	if (!file.is_open())
	{
		return;
	}

	file << Obj.dump(1);
}

void LoadScene(const FString& FilePath)
{
	std::ifstream file(FilePath.c_str());
	if (!file.is_open())
	{
		return;
	}

	// Main Camera 생성
	/*AActor CameraActor;
	CameraActor.AddComponent<USceneComponent>(&CameraActor, FVector(0.0f, 0.0f, 0.f), FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
	CameraActor.AddComponent<UCameraComponent>(&CameraActor, PIDIV2, 0.1f, 1000.f);*/

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	json::JSON Obj = json::JSON::Load(content);
	int32 NextUUID = Obj["NextUUID"].ToInt();
	if (NextUUID > GUDObjectArray.size() + 1)
	{
		GUDObjectArray.reserve(NextUUID - 1);
	}

	json::JSON Primitives = Obj["Primitives"];
	for (const auto& p : Primitives.ObjectRange())
	{
		int Index = std::stoi(p.first);
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
	}
}

AActor* CreateActorFromPrimitive(const FVector& Location, const FVector& Rotation, const FVector& Scale, EPrimitiveType Type)
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
