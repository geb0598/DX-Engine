#include "pch.h"

#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Runtime/Engine/Public/ReferenceSkeleton.h"

IMPLEMENT_ABSTRACT_CLASS(USkinnedMeshComponent, UMeshComponent)

UObject* USkinnedMeshComponent::Duplicate()
{
	/** @todo */
	return nullptr;
}

void USkinnedMeshComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	/** @todo */
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	/** @todo */
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
