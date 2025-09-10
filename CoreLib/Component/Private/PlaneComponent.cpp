#include "Component/Public/PlaneComponent.h"
#include "RayCaster/Raycaster.h"
#include "AssetManager/AssetManager.h"

FVertexSimple PlaneVertices[] =
{
    // Front face (Z+)
    { -0.5f, -0.5f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
    { -0.5f,  0.5f,  0.0f,  1.0f, 1.0f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, -0.5f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
    {  0.5f,  0.5f,  0.0f,  0.0f, 0.0f, 1.0f, 1.0f },
    {  0.5f, -0.5f,  0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
};

UPlaneComponent::UPlaneComponent(AActor* Actor,
    std::shared_ptr<UVertexShader> VertexShader,
    std::shared_ptr<UPixelShader> PixelShader
) : UPrimitiveComponent(Actor, VertexShader, PixelShader)
{
    TArray<FVertex> VertexArray;
    for (size_t i = 0; i < sizeof(PlaneVertices) / sizeof(FVertexSimple); ++i)
    {
        VertexArray.push_back(static_cast<FVertex>(PlaneVertices[i]));
    }

    UAssetManager& AssetManager = UAssetManager::GetInstance();
    Mesh = AssetManager.GetOrCreateMesh("PlaneMesh", VertexArray);
}

UPlaneComponent::EType UPlaneComponent::GetType() const
{
    return EType::Plane;
}

std::optional<float> UPlaneComponent::GetHitResultAtScreenPosition(
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
