#include "pch.h"

#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_ABSTRACT_CLASS(USkinnedMeshComponent, UMeshComponent)

UObject* USkinnedMeshComponent::Duplicate()
{
	USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(Super::Duplicate());
	SkinnedMeshComponent->SkinnedAsset = SkinnedAsset;
	SkinnedMeshComponent->BoneVisibilityStates = BoneVisibilityStates;
	SkinnedMeshComponent->ComponentSpaceTransformArray.SetNum(ComponentSpaceTransformArray.Num());

	return SkinnedMeshComponent;
}

void USkinnedMeshComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		// BoneVisibilityStates 불러오기
		JSON BoneVisibilityJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "BoneVisibilityStates", BoneVisibilityJson, nullptr, false))
		{
			BoneVisibilityStates.Empty();
			for (size_t i = 0; i < BoneVisibilityJson.size(); ++i)
			{
				int32 VisibilityState;
				FJsonSerializer::ReadInt32(BoneVisibilityJson, std::to_string(i), VisibilityState, BVS_Visible, false);
				BoneVisibilityStates.Add(static_cast<uint8>(VisibilityState));
			}
		}
	}
	// 저장
	else
	{
		// BoneVisibilityStates 저장
		if (BoneVisibilityStates.Num() > 0)
		{
			JSON BoneVisibilityJson = json::Object();
			for (int32 i = 0; i < BoneVisibilityStates.Num(); ++i)
			{
				BoneVisibilityJson[std::to_string(i)] = static_cast<int32>(BoneVisibilityStates[i]);
			}
			InOutHandle["BoneVisibilityStates"] = BoneVisibilityJson;
		}
	}
}

void USkinnedMeshComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USkinnedMeshComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	TickPose(DeltaTime);

	RefreshBoneTransforms();
}

void USkinnedMeshComponent::EndPlay()
{
	Super::EndPlay();
}

void USkinnedMeshComponent::TickPose(float DeltaTime)
{
}

void USkinnedMeshComponent::HideBone(int32 BoneIndex)
{
	TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		EditableBoneVisibilityStates[BoneIndex] = BVS_ExplicitlyHidden;
		// RebuildVisibilityArray();
	}
}

void USkinnedMeshComponent::UnHideBone(int32 BoneIndex)
{
	TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		EditableBoneVisibilityStates[BoneIndex] = BVS_Visible;
		// RebuildVisiblityArray();
	}
}

bool USkinnedMeshComponent::IsBoneHidden(int32 BoneIndex)
{
	const TArray<uint8>& EditableBoneVisibilityStates = GetEditableBoneVisibilityStates();
	if (BoneIndex < EditableBoneVisibilityStates.Num())
	{
		if (BoneIndex != INDEX_NONE)
		{
			return EditableBoneVisibilityStates[BoneIndex] != BVS_Visible;
		}
	}
	return false;
}
