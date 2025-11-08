#include "pch.h"

#include "Runtime/Engine/Public/SkeletalMesh.h"

IMPLEMENT_CLASS(USkeletalMesh, USkinnedAsset)

FSkeletalMeshRenderData* USkeletalMesh::GetSkeletalMeshRenderData() const
{
	return SkeletalMeshRenderData.Get();
}

void USkeletalMesh::CalculateInvRefMatrices()
{
	const int32 NumRealBones = GetRefSkeleton().GetRawBoneNum();
}
