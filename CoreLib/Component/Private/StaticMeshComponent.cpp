#include "Component/Public/StaticMeshComponent.h"
#include "RayCaster/Public/URayCaster.h"

UStaticMeshComponent::UStaticMeshComponent(
    AActor* Actor,
    std::shared_ptr<UVertexShader> InVertexShader,
    std::shared_ptr<UPixelShader> InPixelShader,
    std::shared_ptr<UMesh> InMesh)
    : UPrimitiveComponent(Actor, InVertexShader, InPixelShader, InMesh)
{
}
std::optional<float> UStaticMeshComponent::GetHitResultAtScreenPosition(
    URayCaster& RayCaster,
    int32 MouseX,
    int32 MouseY,
    int32 ScreenWidth,
    int32 ScreenHeight,
    const FMatrix& ModelingMatrix,
    const FMatrix& ViewMatrix,
    const FMatrix& ProjectionMatrix)
{
    // RayCaster의 StaticMeshComponent용 오버로드 함수를 호출합니다.
    return RayCaster.GetHitResultAtScreenPosition(
        *this,
        MouseX,
        MouseY,
        ScreenWidth,
        ScreenHeight,
        ModelingMatrix,
        ViewMatrix,
        ProjectionMatrix
    );
}