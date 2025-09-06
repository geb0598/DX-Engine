#pragma once

#include <memory>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"

class UPrimitiveComponent
{
public:
	virtual ~UPrimitiveComponent() = default;

	UPrimitiveComponent(
		UActor* Actor, 
		std::shared_ptr<UMesh> Mesh,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	UPrimitiveComponent(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent(UPrimitiveComponent&&) = delete;

	UPrimitiveComponent& operator=(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent& operator=(UPrimitiveComponent&&) = delete;

	UVertexShader* GetVertexShader();
	UPixelShader* GetPixelShader();

	void Render(ID3D11DeviceContext* DeviceContext);

private:
	std::shared_ptr<UMesh> Mesh;
	std::shared_ptr<UVertexShader> VertexShader;
	std::shared_ptr<UPixelShader> PixelShader;
};
