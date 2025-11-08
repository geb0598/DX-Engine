#include "pch.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"

#include <numeric>

#include "Runtime/Engine/Public/SkeletalMesh.h"

UObject* USkeletalMeshComponent::Duplicate()
{
	/** @todo */
	return nullptr;
}

void USkeletalMeshComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	/** @todo */
}

void USkeletalMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	/** @todo */
}

void USkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();
	/** @todo */
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	/** @todo */
}

void USkeletalMeshComponent::EndPlay()
{
	Super::EndPlay();
	/** @todo */
}

void USkeletalMeshComponent::RefreshBoneTransforms()
{
	if (!GetSkeletalMeshAsset() || GetNumComponentSpaceTransforms() == 0)
	{
		return;
	}

	/** @note LOD 시스템이 없으므로 모든 본을 사용한다. */
	TArray<FBoneIndexType> FillComponentSpaceTransformsRequiredBones(GetSkeletalMeshAsset()->GetRefSkeleton().GetRawBoneNum());
	std::iota(FillComponentSpaceTransformsRequiredBones.begin(), FillComponentSpaceTransformsRequiredBones.end(), 0);
	TArray<FTransform>& EditableSpaceBases = GetEditableComponentSpaceTransform();

	/** 현재는 애니메이션 시스템이 없으므로 FillComponentSpaceTransforms를 직접 호출한다. */
	SkeletalMeshAsset->FillComponentSpaceTransforms(
		BoneSpaceTransforms,
		FillComponentSpaceTransformsRequiredBones,
		EditableSpaceBases
	);
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

void USkeletalMeshComponent::SetSkeletalMeshAsset(USkeletalMesh* NewMesh)
{
	if (NewMesh == GetSkeletalMeshAsset())
	{
		// 이미 사용중인 에셋일 경우, 아무것도 하지않고 반환한다.
		return;
	}

	// 부모 클래스의 멤버(SkinnedAsset)에 에셋을 설정
	SetSkinnedAsset(NewMesh);
	SkeletalMeshAsset = NewMesh;

	if (SkeletalMeshAsset == nullptr)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetRefSkeleton();
		const int32 NumBones = RefSkeleton.GetRawBoneNum();

		BoneSpaceTransforms = RefSkeleton.GetRawRefBonePose();
		GetEditableComponentSpaceTransform().SetNum(NumBones);
		GetEditableBoneVisibilityStates().SetNum(NumBones);
	}
	else
	{
		BoneSpaceTransforms.Empty();
		GetEditableComponentSpaceTransform().Empty();
		GetEditableBoneVisibilityStates().Empty();
	}
}
