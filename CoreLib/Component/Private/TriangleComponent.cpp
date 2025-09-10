#include "Renderer/Renderer.h"
#include "Component/Public/TriangleComponent.h"
#include "AssetManager/AssetManager.h"
#include "RayCaster/Raycaster.h"

FVertexSimple triangle_vertices[] =
{
	{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },  // Bottom-left vertex (blue)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right vertex (green)

	// should be seen on opposite side
	{ 0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right vertex (green)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },  // Bottom-left vertex (blue)
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

	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Mesh = ResourceManager.GetOrCreateMesh("TriangleMesh", VertexArray);
}

UTriangleComponent::EType UTriangleComponent::GetType() const
{
	return EType::Triangle;
}

std::optional<float> UTriangleComponent::GetHitResultAtScreenPosition(
	URayCaster& RayCaster, 
	int32 MouseX,
	int32 MouseY,
	int32 ScreenWidth,
	int32 ScreenHeight, const FMatrix& ModelingMatrix,
	const FMatrix& ViewMatrix, 
	const FMatrix& ProjectionMatrix
)
{
	return RayCaster.GetHitResultAtScreenPosition(
		*this,
		MouseX, 
		MouseY,
		ScreenWidth,
		ScreenHeight,
		ModelingMatrix,
		ViewMatrix,
		ProjectionMatrix
	);
}
