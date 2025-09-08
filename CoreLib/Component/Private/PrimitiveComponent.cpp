#include "Component/Public/PrimitiveComponent.h"
#include "../App/Source/BasicShapes.h"
#include "Renderer/Renderer.h"

UPrimitiveComponent::UPrimitiveComponent(AActor* Actor, EPrimitiveType PrimitiveType,
	std::shared_ptr<UVertexShader> VertexShader,
	std::shared_ptr<UPixelShader> PixelShader
) : UActorComponent(Actor), PrimitiveType(PrimitiveType), VertexShader(VertexShader), PixelShader(PixelShader)
{
	CreateMesh(PrimitiveType);
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

void UPrimitiveComponent::CreateMesh(EPrimitiveType PrimitiveType)
{
	URenderer& Renderer = URenderer::GetInstance();

	TArray<FVertex> VertexArray;
	switch (PrimitiveType)
	{
	case EPrimitiveType::EPT_Triangle:
		for (size_t i = 0; i < sizeof(triangle_vertices) / sizeof(FVertexSimple); ++i)
		{
			VertexArray.push_back(static_cast<FVertex>(triangle_vertices[i]));
		}
		break;
	case EPrimitiveType::EPT_Cube:
		for (size_t i = 0; i < sizeof(cube_vertices) / sizeof(FVertexSimple); ++i)
		{
			VertexArray.push_back(static_cast<FVertex>(cube_vertices[i]));
		}
		break;
	case EPrimitiveType::EPT_Sphere:
		for (size_t i = 0; i < sizeof(sphere_vertices) / sizeof(FVertexSimple); ++i)
		{
			VertexArray.push_back(static_cast<FVertex>(sphere_vertices[i]));
		}
		break;
	}

	Mesh = std::make_shared<UMesh>(Renderer.GetDevice(), VertexArray);
}
