#pragma once
#include "Math/Math.h"
#include "Containers/Containers.h"
#include "Json/json.hpp"
#include "Object/Object.h"
#include "Actor/Actor.h"
#include "Component/Component.h"
#include <fstream>

// TODO: 임시 Primitive Type, 만들어줘야함.
enum class EPrimitiveType
{
	Sphere,
	Cube,
	Triangle
};

FString PrimitiveTypeToString(EPrimitiveType Type)
{
	switch (Type)
	{
	case EPrimitiveType::Sphere:
		return "Sphere";
	case EPrimitiveType::Cube:
		return "Cube";
	case EPrimitiveType::Triangle:
		return "Triangle";
	default:
		return "Unknown";
	}
}

void SavePrimitive(json::JSON& Obj, int Index, FVector Location, FVector Rotation, FVector Scale, EPrimitiveType Type);
void SaveScene(const FString& FilePath, int32 Version);
void LoadScene(const FString& FilePath);