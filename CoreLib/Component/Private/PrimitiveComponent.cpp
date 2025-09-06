#include "Component/Public/PrimitiveComponent.h"

UPrimitiveComponent::UPrimitiveComponent(
	UActor* Actor, 
	std::shared_ptr<UMesh> Mesh,
	std::shared_ptr<UVertexShader> VertexShader,
	std::shared_ptr<UPixelShader> PixelShader
) : Mesh(Mesh), VertexShader(VertexShader), PixelShader(PixelShader)
{

}

void UPrimitiveComponent::Render(ID3D11DeviceContext* DeviceContext)
{
	// TODO;
	// VertexShader->Bind(DeviceContext, TODO:cbuffer)
	// PixelShader->Bind(DeviceContext, TODO:cbuffer)

	//Mesh->Bind(DeviceContext);

	//DeviceContext->Draw();
}
