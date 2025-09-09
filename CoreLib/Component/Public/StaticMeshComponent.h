#pragma once
#include "Component/Public/PrimitiveComponent.h"

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
};