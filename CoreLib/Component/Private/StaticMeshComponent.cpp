#include "Component/Public/StaticMeshComponent.h"

UStaticMeshComponent::UStaticMeshComponent(
    AActor* Actor,
    std::shared_ptr<UVertexShader> InVertexShader,
    std::shared_ptr<UPixelShader> InPixelShader,
    std::shared_ptr<UMesh> InMesh)
    : UPrimitiveComponent(Actor, InVertexShader, InPixelShader, InMesh)
{
}