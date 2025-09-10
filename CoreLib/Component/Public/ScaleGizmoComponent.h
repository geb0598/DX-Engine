#pragma once
#include "Component/Public/GizmoComponent.h"

class AActor;
class UStaticMeshComponent;

class UScaleGizmoComponent : public UGizmoComponent
{
public:
    UScaleGizmoComponent(AActor* Actor);
    virtual ~UScaleGizmoComponent();

    virtual void HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj) override;
    virtual void Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj) override;

private:
    AActor* XAxisActor;
    AActor* YAxisActor;
    AActor* ZAxisActor;

    FVector DragStartPoint_World;
    FVector DragStartActorScale;

    FVector DragPlaneNormal;
};