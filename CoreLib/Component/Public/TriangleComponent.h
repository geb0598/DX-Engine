#pragma once

#include <memory>

#include <d3d11.h>

#include "PrimitiveComponent.h"

class UTriangleComponent : public UPrimitiveComponent
{
public:
	virtual ~UTriangleComponent() = default;

	UTriangleComponent(AActor* Actor,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	UTriangleComponent(const UTriangleComponent&) = delete;
	UTriangleComponent(UTriangleComponent&&) = delete;

	UTriangleComponent& operator=(const UTriangleComponent&) = delete;
	UTriangleComponent operator=(UTriangleComponent&&) = delete;

	virtual EType GetType() const override;
};
