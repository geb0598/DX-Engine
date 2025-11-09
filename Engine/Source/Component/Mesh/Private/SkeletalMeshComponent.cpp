#include "pch.h"

#include <numeric>

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Runtime/Engine/Public/SkeletalMesh.h"

IMPLEMENT_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

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

	UpdateSkinnedVertices();
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

	if (!bPoseDirty)
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

	const TArray<FMatrix>& InvBindMatrices = SkeletalMeshAsset->GetRefBasesInvMatrix();

	for (int32 i = 0; i < FillComponentSpaceTransformsRequiredBones.Num(); ++i)
	{
		const int32 BoneIndex = FillComponentSpaceTransformsRequiredBones[i];

		/** 스키닝 행렬 = (모델 공간 -> 본 공간) * (본 공간 -> 포즈 모델 공간) */
		SkinningMatrices[BoneIndex] = InvBindMatrices[BoneIndex] * EditableSpaceBases[BoneIndex].ToMatrixWithScale();
	}

	bPoseDirty = false;
	bSkinningDirty = true;
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

	if (SkeletalMeshAsset)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetRefSkeleton();
		const int32 NumBones = RefSkeleton.GetRawBoneNum();
		const int32 NumVertices = SkeletalMeshAsset->GetStaticMesh()->GetVertices().Num();

		BoneSpaceTransforms = RefSkeleton.GetRawRefBonePose();
		SkinnedVertices.SetNum(NumVertices);
		GetEditableComponentSpaceTransform().SetNum(NumBones);
		GetEditableBoneVisibilityStates().SetNum(NumBones);

		bPoseDirty = true;
		bSkinningDirty = true;
	}
	else
	{
		BoneSpaceTransforms.Empty();
		SkinnedVertices.Empty();
		GetEditableComponentSpaceTransform().Empty();
		GetEditableBoneVisibilityStates().Empty();
	}
}

FTransform USkeletalMeshComponent::GetBoneTransformLocal(int32 BoneIndex)
{
	return BoneSpaceTransforms[BoneIndex];
}

void USkeletalMeshComponent::SetBoneTransformLocal(int32 BoneIndex, const FTransform& NewLocalTransform)
{
	if (BoneSpaceTransforms.IsValidIndex(BoneIndex))
	{
		BoneSpaceTransforms[BoneIndex] = NewLocalTransform;
		bPoseDirty = true;
	}
}

void USkeletalMeshComponent::UpdateSkinnedVertices()
{
	if (!bSkinningDirty)
	{
		return;
	}

	if (!SkeletalMeshAsset)
	{
		return;
	}

	FSkeletalMeshRenderData* RenderData = SkeletalMeshAsset->GetSkeletalMeshRenderData();
	const TArray<FNormalVertex>& Vertices = GetSkeletalMeshAsset()->GetStaticMesh()->GetVertices();
	const TArray<FRawSkinWeight>& SkinWeights = RenderData->SkinWeightVertices;

	for (int32 VertexIndex = 0; VertexIndex < Vertices.Num(); ++VertexIndex)
	{
		const FNormalVertex& Vertex = Vertices[VertexIndex];
		const FRawSkinWeight& SkinWeight = SkinWeights[VertexIndex];

		FVector FinalPosition(0.0f, 0.0f, 0.0f);
		FVector FinalNormal(0.0f, 0.0f, 0.0f);
		FVector FinalTangent(0.0f, 0.0f, 0.0f);

		uint32 TotalWeight = 0;
		for (int32 InfluenceIndex = 0; InfluenceIndex < FRawSkinWeight::MAX_TOTAL_INFLUENCES; ++InfluenceIndex)
		{
			const FBoneIndexType BoneIndex = SkinWeight.InfluenceBones[InfluenceIndex];
			const uint16 Weight = SkinWeight.InfluenceWeights[InfluenceIndex];
			TotalWeight += Weight;

			if (Weight == 0)
			{
				continue;
			}

			const FMatrix& FinalMatrix = SkinningMatrices[BoneIndex];

			FinalPosition += FinalMatrix.TransformPosition(Vertex.Position) * Weight;
			FinalNormal += FinalMatrix.TransformVector(Vertex.Normal) * Weight;
			// @todo FVector4 타입 탄젠트 처리 (@박영빈 해주세요)
			// FinalTangent += FinalMatrix.TransformVector(Vertex.Tangent) * Weight;
		}

		FNormalVertex& ResultVertex = SkinnedVertices[VertexIndex];
		ResultVertex.Position = FinalPosition / TotalWeight;
		ResultVertex.Normal = FinalNormal / TotalWeight;
		// @todo FVector4 타입 탄젠트 처리 (@박영빈 해주세요)
		// ResultVertex.Tangent = FinalTangent / TotalWeight;
	}

	bSkinningDirty = false;
}

void USkeletalMeshComponent::EnableNormalMap()
{
	bNormalMapEnabled = true;
}

void USkeletalMeshComponent::DisableNormalMap()
{
	bNormalMapEnabled = false;
}

bool USkeletalMeshComponent::IsNormalMapEnabled() const
{
	return bNormalMapEnabled;
}
