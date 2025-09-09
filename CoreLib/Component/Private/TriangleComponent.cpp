#include "Renderer/Renderer.h"
#include "Component/Public/TriangleComponent.h"

FVertexSimple triangle_vertices[] =
{
	{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },  // Bottom-left vertex (blue)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f } // Bottom-right vertex (green)
};

UTriangleComponent::UTriangleComponent(AActor* Actor, 
	std::shared_ptr<UVertexShader> VertexShader, 
	std::shared_ptr<UPixelShader> PixelShader
) : UPrimitiveComponent(Actor, VertexShader, PixelShader)
{
	TArray<FVertex> VertexArray;
	for (size_t i = 0; i < sizeof(triangle_vertices) / sizeof(FVertexSimple); ++i)
	{
		VertexArray.push_back(static_cast<FVertex>(triangle_vertices[i]));
	}

	auto& Renderer = URenderer::GetInstance();
	Mesh = std::make_shared<UMesh>(Renderer.GetDevice(), VertexArray);
}

UTriangleComponent::EType UTriangleComponent::GetType() const
{
	return EType::Triangle;
}
