#include "pch.h"
#include "BoneAnchorComponent.h"
#include "SelectionManager.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"

IMPLEMENT_CLASS(UBoneAnchorComponent)

void UBoneAnchorComponent::SetTarget(USkeletalMeshComponent* InTarget, int32 InBoneIndex)
{
    Target = InTarget;
    BoneIndex = InBoneIndex;
    UpdateAnchorFromBone();
}

void UBoneAnchorComponent::UpdateAnchorFromBone()
{
    if (!Target || BoneIndex < 0)
        return;

    const FTransform BoneWorld = Target->GetBoneWorldTransform(BoneIndex);
    SetWorldTransform(BoneWorld) ;
}

void UBoneAnchorComponent::OnTransformUpdated()
{
    Super::OnTransformUpdated();

    if (!Target || BoneIndex < 0)
        return;

    const FTransform AnchorWorld = GetWorldTransform();
    Target->SetBoneWorldTransform(BoneIndex, AnchorWorld);

    // 편집된 bone transform을 캐시에 저장 (애니메이션 재생 중에도 유지)
    if (State)
    {
        const FTransform BoneLocal = Target->GetBoneLocalTransform(BoneIndex);
        State->EditedBoneTransforms[BoneIndex] = BoneLocal;

        // Bone line 실시간 업데이트를 위해 dirty 플래그 설정
        State->bBoneLinesDirty = true;
    }
}
