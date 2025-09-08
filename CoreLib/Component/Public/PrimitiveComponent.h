#pragma once

#include <memory>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"

enum class EPrimitiveType
{
	EPT_Triangle,
	EPT_Cube,
	EPT_Sphere,
	EPT_Max,
};

class AActor;

class UPrimitiveComponent : public UActorComponent
{
public:
	virtual ~UPrimitiveComponent() = default;

	UPrimitiveComponent(AActor* Actor, 
		EPrimitiveType PrimitiveType,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader
	);

	UPrimitiveComponent(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent(UPrimitiveComponent&&) = delete;

	UPrimitiveComponent& operator=(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent& operator=(UPrimitiveComponent&&) = delete;

	UVertexShader* GetVertexShader();
	UPixelShader* GetPixelShader();
	EPrimitiveType GetPrimitiveType() const { return PrimitiveType; }

	void Render(ID3D11DeviceContext* DeviceContext);

private:
	void CreateMesh(EPrimitiveType PrimitiveType);

private:
	EPrimitiveType PrimitiveType;

	std::shared_ptr<UMesh> Mesh;
	std::shared_ptr<UVertexShader> VertexShader;
	std::shared_ptr<UPixelShader> PixelShader;
};
