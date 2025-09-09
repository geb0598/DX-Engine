#include "Component/Public/PrimitiveComponent.h"
#include "RayCaster/Raycaster.h"

UPrimitiveComponent::UPrimitiveComponent(AActor* Actor)
	: UActorComponent(Actor)
{

}

UPrimitiveComponent::UPrimitiveComponent(AActor* Actor,
	std::shared_ptr<UVertexShader> VertexShader,
	std::shared_ptr<UPixelShader> PixelShader,
	std::shared_ptr<UMesh> Mesh
) : 
	UActorComponent(Actor), 
	VertexShader(VertexShader), 
	PixelShader(PixelShader),
	Mesh(Mesh)
{

}

UMesh* UPrimitiveComponent::GetMesh()
{
	return Mesh.get();
}

UVertexShader* UPrimitiveComponent::GetVertexShader()
{
	return VertexShader.get();
}

UPixelShader* UPrimitiveComponent::GetPixelShader()
{
	return PixelShader.get();
}

UPrimitiveComponent::EType UPrimitiveComponent::GetType() const
{
	return EType::Primitive;
}

std::optional<float> UPrimitiveComponent::GetHitResultAtScreenPosition(
	URayCaster& RayCaster,
	int32 MouseX,
	int32 MouseY,
	int32 ScreenWidth,
	int32 ScreenHeight,
	const FMatrix& ModelingMatrix, 
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	return std::nullopt;
}

void UPrimitiveComponent::Render(ID3D11DeviceContext* DeviceContext)
{
	// TODO: LOG or WARN users that mesh information is not properly provided
	if (!Mesh)
	{
		return;
	}

	Mesh->Bind(DeviceContext);

	DeviceContext->Draw(Mesh->GetVertexCount(), 0);
}
