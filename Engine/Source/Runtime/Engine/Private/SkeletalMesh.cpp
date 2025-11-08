#include "pch.h"

#include "Runtime/Engine/Public/SkeletalMesh.h"

IMPLEMENT_CLASS(USkeletalMesh, USkinnedAsset)

FSkeletalMeshRenderData* USkeletalMesh::GetSkeletalMeshRenderData() const
{
	return SkeletalMeshRenderData.Get();
}

void USkeletalMesh::SetSkeletalMeshRenderData(FSkeletalMeshRenderData* InRenderData)
{
	SkeletalMeshRenderData.Reset(InRenderData);
}

const TArray<FSkeletalVertex>& USkeletalMesh::GetVertices() const
{
	return SkeletalMeshRenderData->SkeletalVertices;
}

const TArray<uint32>& USkeletalMesh::GetIndices() const
{
	return SkeletalMeshRenderData->Indices;
}

void USkeletalMesh::CalculateInvRefMatrices()
{
	const int32 NumRealBones = GetRefSkeleton().GetRawBoneNum();
}
