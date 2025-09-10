#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include <optional>

class UStaticMeshComponent : public UPrimitiveComponent
{
public:
    UStaticMeshComponent(
        AActor* Actor,
        std::shared_ptr<UVertexShader> InVertexShader,
        std::shared_ptr<UPixelShader> InPixelShader,
        std::shared_ptr<UMesh> InMesh
    );

    virtual EType GetType() const override { return EType::Primitive; }

    virtual std::optional<float> GetHitResultAtScreenPosition(
        URayCaster& RayCaster,
        int32 MouseX,
        int32 MouseY,
        int32 ScreenWidth,
        int32 ScreenHeight,
        const FMatrix& ModelingMatrix,
        const FMatrix& ViewMatrix,
        const FMatrix& ProjectionMatrix
    ) override;
};