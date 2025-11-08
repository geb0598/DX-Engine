#include "pch.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"

void USkeletalMeshComponent::RefreshBoneTransforms()
{
}

void USkeletalMeshComponent::TickPose(float DeltaTime)
{
	Super::TickPose(DeltaTime);

	// @todo TickAnimation
}

USkeletalMesh* USkeletalMeshComponent::GetSkeletalMeshAsset() const
{
	return Cast<USkeletalMesh>(GetSkinnedAsset());
}
