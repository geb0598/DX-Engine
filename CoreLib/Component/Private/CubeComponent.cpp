#include "Component/Public/CubeComponent.h"
#include "RayCaster/Raycaster.h"
#include "AssetManager/AssetManager.h"

FVertexSimple cube_vertices[] =
{
    // Front face (Z+)
    { -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },

    // Back face (Z-)
    {  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, 1.0f },

    // Left face (X-)
    { -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },

    // Right face (X+)
    {  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f },
    {  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f, 0.5f, 0.0f, 1.0f },

    // Top face (Y+)
    { -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.5f, 1.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f, 0.5f, 1.0f, 1.0f },
    { -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f, 0.5f, 1.0f, 1.0f },
    {  0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.0f, 1.0f },

    // Bottom face (Y-)
    { -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.0f, 1.0f },
    { -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.0f, 1.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f },
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

	UAssetManager& AssetManager = UAssetManager::GetInstance();
	Mesh = AssetManager.GetOrCreateMesh("CubeMesh", VertexArray);
}

UCubeComponent::EType UCubeComponent::GetType() const
{
	return EType::Cube;
}

std::optional<float> UCubeComponent::GetHitResultAtScreenPosition(
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
