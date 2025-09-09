#pragma once

#include <memory>

#include <d3d11.h>

#include "PrimitiveComponent.h"

class UCubeComponent : public UPrimitiveComponent
{
public:
	virtual ~UCubeComponent() = default;

	UCubeComponent(AActor* Actor,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	UCubeComponent(const UCubeComponent&) = delete;
	UCubeComponent(UCubeComponent&&) = delete;

	UCubeComponent& operator=(const UCubeComponent&) = delete;
	UCubeComponent operator=(UCubeComponent&&) = delete;

	virtual EType GetType() const override;
};
