#include "Component/Public/PrimitiveComponent.h"

UPrimitiveComponent::UPrimitiveComponent(
	AActor* Actor, 
	std::shared_ptr<UMesh> Mesh,
	std::shared_ptr<UVertexShader> VertexShader,
	std::shared_ptr<UPixelShader> PixelShader
) : UActorComponent(Actor), Mesh(Mesh), VertexShader(VertexShader), PixelShader(PixelShader)
{

}

UVertexShader* UPrimitiveComponent::GetVertexShader()
{
	return VertexShader.get();
}

UPixelShader* UPrimitiveComponent::GetPixelShader()
{
	return PixelShader.get();
}

void UPrimitiveComponent::Render(ID3D11DeviceContext* DeviceContext)
{
	Mesh->Bind(DeviceContext);

	DeviceContext->Draw(Mesh->GetVertexCount(), 0);
}
