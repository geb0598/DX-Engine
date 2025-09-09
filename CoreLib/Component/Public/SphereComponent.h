#pragma once

#include <memory>

#include <d3d11.h>

#include "PrimitiveComponent.h"

class USphereComponent : public UPrimitiveComponent
{
public:
	virtual ~USphereComponent() = default;

	USphereComponent(AActor* Actor,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	USphereComponent(const USphereComponent&) = delete;
	USphereComponent(USphereComponent&&) = delete;

	USphereComponent& operator=(const USphereComponent&) = delete;
	USphereComponent operator=(USphereComponent&&) = delete;

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
