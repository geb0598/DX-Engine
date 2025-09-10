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
    //   Ÿ  ͵
    AActor* XAxisActor;
    AActor* YAxisActor;
    AActor* ZAxisActor;

    // 巡   Ǵ 
    FVector DragStartPoint_World;
    FVector DragStartActorLocation;

    // [추가] 드래그가 시작될 때 생성되는 가상 평면의 법선 벡터
    FVector DragPlaneNormal;
};