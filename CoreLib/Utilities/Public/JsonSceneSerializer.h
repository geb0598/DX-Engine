#pragma once
#include "Math/Math.h"
#include "Containers/Containers.h"
#include "Object/Object.h"
#include "Actor/Actor.h"
#include "Component/Component.h"
#include <fstream>

namespace json { class JSON; }

enum class ETypePrimitive
{
	EPT_Triangle,
	EPT_Cube,
	EPT_Sphere,
	EPT_Max,
};

FString PrimitiveTypeToString(ETypePrimitive Type);
ETypePrimitive StringToPrimitiveType(const FString& TypeStr);

void SavePrimitive(json::JSON& Obj, int Index, FVector Location, FVector Rotation, FVector Scale, ETypePrimitive Type);
void SaveScene(const FString& FilePath, int32 Version);
void LoadScene(const FString& FilePath, HWND hWnd);

AActor* CreateActorFromPrimitive(const FVector& Location, const FVector& Rotation, const FVector& Scale, ETypePrimitive Type, HWND hWnd);