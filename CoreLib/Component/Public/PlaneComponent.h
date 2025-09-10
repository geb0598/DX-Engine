#pragma once

#include <memory>

#include <d3d11.h>

#include "PrimitiveComponent.h"

extern FVertexSimple PlaneVertices[];

class UPlaneComponent : public UPrimitiveComponent
{
public:
	virtual ~UPlaneComponent() = default;

	UPlaneComponent(AActor* Actor,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	UPlaneComponent(const UPlaneComponent&) = delete;
	UPlaneComponent(UPlaneComponent&&) = delete;

	UPlaneComponent& operator=(const UPlaneComponent&) = delete;
	UPlaneComponent operator=(UPlaneComponent&&) = delete;

	virtual EType GetType() const override;

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