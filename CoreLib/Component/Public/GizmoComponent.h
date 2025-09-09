#pragma once
#include "Component/Public/USceneComponent.h"
#include "Containers/Containers.h"

class URayCaster;
class UWindow;
class URenderer;

enum class EAxis
{
    None,
    X,
    Y,
    Z
};

class UGizmoComponent : public USceneComponent
{
public:
    UGizmoComponent(AActor* Actor);
    virtual ~UGizmoComponent() = default;

    virtual void SetTarget(AActor* Target);
    virtual void HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj) = 0;
    virtual void Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj) = 0;
    bool IsDragging() const;

protected:
    AActor* TargetActor = nullptr;
    EAxis ActiveAxis = EAxis::None;
    bool bIsDragging = false;
};