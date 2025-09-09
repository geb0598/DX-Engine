#include "Component/Public/LocationGizmoComponent.h"
#include "Component/Public/StaticMeshComponent.h"
#include "Mesh/Public/GizmoShapes.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "RayCaster/Raycaster.h"
#include "Utilities/Utilities.h"
#include <limits>

ULocationGizmoComponent::ULocationGizmoComponent(AActor* Actor)
    : UGizmoComponent(Actor)
{
    URenderer& Renderer = URenderer::GetInstance();
    ID3D11Device* Device = Renderer.GetDevice();

    TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc(std::begin(FVertex::InputLayoutDesc), std::end(FVertex::InputLayoutDesc));
    auto GizmoVertexShader = std::make_shared<UVertexShader>(Device, "./Shader/VertexShader.hlsl", "main", LayoutDesc);
    auto GizmoPixelShader = std::make_shared<UPixelShader>(Device, "./Shader/PixelShader.hlsl", "main");

    // X-Axis Actor
    XAxisActor = new AActor();
    TArray<FVertex> VerticesX;
    for (const auto& v : XAxisArrowVertices) { VerticesX.push_back(static_cast<FVertex>(v)); }
    auto XMesh = std::make_shared<UMesh>(Device, VerticesX);
    XAxisActor->AddComponent<UStaticMeshComponent>(XAxisActor, GizmoVertexShader, GizmoPixelShader, XMesh);
    XAxisActor->AddComponent<USceneComponent>(XAxisActor);

    // Y-Axis Actor
    YAxisActor = new AActor();
    TArray<FVertex> VerticesY;
    for (const auto& v : YAxisArrowVertices) { VerticesY.push_back(static_cast<FVertex>(v)); }
    auto YMesh = std::make_shared<UMesh>(Device, VerticesY);
    YAxisActor->AddComponent<UStaticMeshComponent>(YAxisActor, GizmoVertexShader, GizmoPixelShader, YMesh);
    YAxisActor->AddComponent<USceneComponent>(YAxisActor);

    // Z-Axis Actor
    ZAxisActor = new AActor();
    TArray<FVertex> VerticesZ;
    for (const auto& v : ZAxisArrowVertices) { VerticesZ.push_back(static_cast<FVertex>(v)); }
    auto ZMesh = std::make_shared<UMesh>(Device, VerticesZ);
    ZAxisActor->AddComponent<UStaticMeshComponent>(ZAxisActor, GizmoVertexShader, GizmoPixelShader, ZMesh);
    ZAxisActor->AddComponent<USceneComponent>(ZAxisActor);
}

ULocationGizmoComponent::~ULocationGizmoComponent()
{
    delete XAxisActor;
    delete YAxisActor;
    delete ZAxisActor;
}

void ULocationGizmoComponent::Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj)
{
    if (!TargetActor) return;

    auto* DeviceContext = Renderer.GetDeviceContext();
    auto* DepthStencilView = Renderer.GetDepthStencilView();

    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
    SetLocation(TargetSceneComp->GetLocation());

    auto CameraLocation = USceneManager::GetInstance().GetMainCameraActor()->GetComponent<USceneComponent>()->GetLocation();
    float Distance = (GetLocation() - CameraLocation).Length();
    float ScaleFactor = Distance * 0.07f; // 크기 보정 상수
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    AActor* Axes[] = { XAxisActor, YAxisActor, ZAxisActor };
    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = Axes[i]->GetComponent<USceneComponent>();
        auto PrimComp = Axes[i]->GetComponent<UPrimitiveComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());
        // Y축과 Z축은 모델 자체를 회전시켜 방향을 맞춤
        if (i == 1) SceneComp->SetRotation({ 0, 0, 90 }); // Y-axis
        if (i == 2) SceneComp->SetRotation({ 0, -90, 0 }); // Z-axis

        FMatrix ModelMatrix = SceneComp->GetModelingMatrix();
        FMatrix MVP = ModelMatrix * View * Proj;

        int bIsSelected = (static_cast<int>(ActiveAxis) == i + 1) ? 1 : 0;
        PrimComp->GetVertexShader()->UpdateConstantBuffer(DeviceContext, "constants", &MVP);
        PrimComp->GetPixelShader()->UpdateConstantBuffer(DeviceContext, "constants", &bIsSelected);

        PrimComp->GetVertexShader()->Bind(DeviceContext, "constants");
        PrimComp->GetPixelShader()->Bind(DeviceContext, "constants");

        PrimComp->Render(DeviceContext);
    }
}

void ULocationGizmoComponent::HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj)
{
    // 로직은 다음 답변에서...
}
// [추가] GetDepthStencilView 함수의 구현
ID3D11DepthStencilView* URenderer::GetDepthStencilView()
{
    return DepthStencilView.Get();
}