#pragma once
#include "Component/Public/GizmoComponent.h"

// Forward Declaration
class UStaticMeshComponent;
class AActor;
class URenderer;

class URotationGizmoComponent : public UGizmoComponent
{
public:
    URotationGizmoComponent(AActor* Actor);
    virtual ~URotationGizmoComponent() override; // = default 삭제하고 선언만 남깁니다.

    virtual void HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj) override;
    virtual void Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj) override;
   
private:
    AActor* RingActorX = nullptr;
    AActor* RingActorY = nullptr;
    AActor* RingActorZ = nullptr;
    AActor* RingActorW = nullptr;

    UStaticMeshComponent* RotationMeshX = nullptr;
    UStaticMeshComponent* RotationMeshY = nullptr;
    UStaticMeshComponent* RotationMeshZ = nullptr;

    FVector DragStartPoint_World;
    FVector DragStartActorRotation;
    float DragStartMouseX = 0.0f;
    float DragStartMouseY = 0.0f;

    void CreateGizmoActors(URenderer& Renderer);

};