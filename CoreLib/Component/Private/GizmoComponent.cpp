#include "Component/Public/GizmoComponent.h"

UGizmoComponent::UGizmoComponent(AActor* Actor)
    : USceneComponent(Actor)
{
}

void UGizmoComponent::SetTarget(AActor* Target)
{
    TargetActor = Target;
    if (!TargetActor)
    {
        // 타겟이 없으면 드래그 상태 강제 해제
        bIsDragging = false;
        ActiveAxis = EAxis::None;
    }
}

bool UGizmoComponent::IsDragging() const
{
    return bIsDragging;
}