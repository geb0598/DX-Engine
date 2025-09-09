#pragma once
#include "Component/Public/GizmoComponent.h"
#include <memory>

class AActor;
class UStaticMeshComponent;

class ULocationGizmoComponent : public UGizmoComponent
{
public:
    ULocationGizmoComponent(AActor* Actor);
    virtual ~ULocationGizmoComponent();

    virtual void HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj) override;
    virtual void Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj) override;

private:
    // 기즈모 축을 나타내는 별도의 액터들
    AActor* XAxisActor;
    AActor* YAxisActor;
    AActor* ZAxisActor;

    // 드래그 시작 시 계산되는 정보
    FVector DragStartPoint_World;
    FVector DragStartActorLocation;
};