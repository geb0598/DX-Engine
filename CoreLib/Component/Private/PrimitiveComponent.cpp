#include "Component/Public/PrimitiveComponent.h"

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
