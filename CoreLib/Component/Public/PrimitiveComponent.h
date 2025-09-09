#pragma once

#include <memory>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"

class AActor;

class UPrimitiveComponent : public UActorComponent
{
public:
	enum class EType
	{
		Primitive,
		Triangle,
		Cube,
		Sphere,
		Max
	};

public:
	virtual ~UPrimitiveComponent() = default;

	UPrimitiveComponent(AActor* Actor);

	UPrimitiveComponent(AActor* Actor,
		std::shared_ptr<UVertexShader> VertexShader,
		std::shared_ptr<UPixelShader> PixelShader,
		std::shared_ptr<UMesh> Mesh = nullptr
	);

	UPrimitiveComponent(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent(UPrimitiveComponent&&) = delete;

	UPrimitiveComponent& operator=(const UPrimitiveComponent&) = delete;
	UPrimitiveComponent& operator=(UPrimitiveComponent&&) = delete;

	UMesh* GetMesh();
	UVertexShader* GetVertexShader();
	UPixelShader* GetPixelShader();

	virtual EType GetType() const;

	void Render(ID3D11DeviceContext* DeviceContext);

protected:
	std::shared_ptr<UVertexShader> VertexShader;
	std::shared_ptr<UPixelShader> PixelShader;
	std::shared_ptr<UMesh> Mesh;
};
