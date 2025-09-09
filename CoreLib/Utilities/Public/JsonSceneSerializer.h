#pragma once
#include "Math/Math.h"
#include "Containers/Containers.h"
#include "Object/Object.h"
#include "Actor/Actor.h"
#include "Component/Component.h"
#include <fstream>

namespace json { class JSON; }

FString PrimitiveTypeToString(UPrimitiveComponent::EType Type);
UPrimitiveComponent::EType StringToPrimitiveType(const FString& TypeStr);

void NewScene();
void SaveScene(const FString& FilePath, int32 Version);
void SavePrimitive(json::JSON& Obj, int Index, FVector Location, FVector Rotation, FVector Scale, UPrimitiveComponent::EType Type);
void LoadScene(const FString& FilePath);

AActor * CreateActorFromPrimitive(const FVector& Location, const FVector& Rotation, const FVector& Scale, UPrimitiveComponent::EType Type);