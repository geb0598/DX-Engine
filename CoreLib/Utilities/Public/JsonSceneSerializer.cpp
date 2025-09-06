#include "../Public/JsonSceneSerializer.h"

void SavePrimitive(json::JSON& Obj, int Index, FVector Location, FVector Rotation, FVector Scale, EPrimitiveType Type)
{
	FString IndexStr = std::to_string(Index);
	Obj["Primitives"][IndexStr] = json::Object();
	Obj["Primitives"][IndexStr]["Location"] = json::Array(Location.X, Location.Y, Location.Z);
	Obj["Primitives"][IndexStr]["Rotation"] = json::Array(Rotation.X, Rotation.Y, Rotation.Z);
	Obj["Primitives"][IndexStr]["Scale"] = json::Array(Scale.X, Scale.Y, Scale.Z);
	Obj["Primitives"][IndexStr]["Type"] = PrimitiveTypeToString(Type);
}

void SaveScene(const FString& FilePath, int32 Version)
{
	json::JSON Obj = json::Object();
	Obj["Version"] = Version;
	Obj["NextUUID"] = GUDObjectArray.size() + 1;
	Obj["Primitives"] = json::Object();
	for (size_t i = 0; i < GUDObjectArray.size(); ++i)
	{
		auto Actor = static_cast<AActor*>(GUDObjectArray[i]);
		if (Actor)
		{
			auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
			if (PrimitiveComponent)
			{
				FVector Location = FVector(i, i, i);
				FVector Rotation = FVector(i, i, i);
				FVector Scale = FVector(i, i, i);
				EPrimitiveType Type = EPrimitiveType::Triangle; // Default to Triangle
				SavePrimitive(Obj, static_cast<int>(i), Location, Rotation, Scale, Type);
			}
		}
	}

	std::ofstream file(FilePath.c_str());
	file << Obj.dump(1);
}

// 미완성 함수
void LoadScene(const FString& FilePath)
{
	std::ifstream file(FilePath.c_str());
	if (!file.is_open())
	{
		return;
	}
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	json::JSON Obj = json::JSON::Load(content);
	int32 NextUUID = Obj["NextUUID"].ToInt();
	if (NextUUID > GUDObjectArray.size() + 1)
	{
		GUDObjectArray.resize(NextUUID - 1, nullptr);
	}

	json::JSON Primitives = Obj["Primitives"];
	for (const auto& p : Primitives.ObjectRange())
	{
		int Index = std::stoi(p.first);
		json::JSON Primitive = p.second;

		FVector Location = FVector(
			Primitive["Location"][0].ToFloat(),
			Primitive["Location"][1].ToFloat(),
			Primitive["Location"][2].ToFloat()
		);
		FVector Rotation = FVector(
			Primitive["Rotation"][0].ToFloat(),
			Primitive["Rotation"][1].ToFloat(),
			Primitive["Rotation"][2].ToFloat()
		);
		FVector Scale = FVector(
			Primitive["Scale"][0].ToFloat(),
			Primitive["Scale"][1].ToFloat(),
			Primitive["Scale"][2].ToFloat()
		);
	}
}
