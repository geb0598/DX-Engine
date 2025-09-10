#pragma once

#include <memory>
#include <optional>

#include <d3d11.h>

#include "Component/Public/ActorComponent.h"
#include "Math/Math.h"
#include "Mesh/Mesh.h"
#include "Shader/Shader.h"
#include "Types/Types.h"

// Forward Declaration
class AActor;
class URayCaster;

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
	// NOTE: Visitor Pattern using double dispatch
	virtual std::optional<float> GetHitResultAtScreenPosition(
		URayCaster& RayCaster,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);

	void Render(ID3D11DeviceContext* DeviceContext);

protected:
	std::shared_ptr<UVertexShader> VertexShader;
	std::shared_ptr<UPixelShader> PixelShader;
	std::shared_ptr<UMesh> Mesh;
};
