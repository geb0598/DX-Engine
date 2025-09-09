#include "Renderer/Renderer.h"
#include "Component/Public/CubeComponent.h"

FVertexSimple cube_vertices[] =
{
	// Front face (Z+)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
	{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f },

	// Back face (Z-)
	{ -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },

	// Left face (X-)
	{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },

	// Right face (X+)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.5f, 0.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f },
	{  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f },
	{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.5f, 1.0f },
	{  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f },

	// Top face (Y+)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.5f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  0.0f, 0.5f, 1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  0.0f, 0.5f, 1.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f },
	{  0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.0f, 1.0f },

	// Bottom face (Y-)
	{ -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f },
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f },
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
};

UCubeComponent::UCubeComponent(AActor* Actor, 
	std::shared_ptr<UVertexShader> VertexShader, 
	std::shared_ptr<UPixelShader> PixelShader
) : UPrimitiveComponent(Actor, VertexShader, PixelShader)
{
	TArray<FVertex> VertexArray;
	for (size_t i = 0; i < sizeof(cube_vertices) / sizeof(FVertexSimple); ++i)
	{
		VertexArray.push_back(static_cast<FVertex>(cube_vertices[i]));
	}

	auto& Renderer = URenderer::GetInstance();
	Mesh = std::make_shared<UMesh>(Renderer.GetDevice(), VertexArray);
}

UCubeComponent::EType UCubeComponent::GetType() const
{
	return EType::Cube;
}
