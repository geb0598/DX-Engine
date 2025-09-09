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
    float ScaleFactor = Distance * 0.1f; // 크기 보정 상수
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    AActor* Axes[] = { XAxisActor, YAxisActor, ZAxisActor };
    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = Axes[i]->GetComponent<USceneComponent>();
        auto PrimComp = Axes[i]->GetComponent<UStaticMeshComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());
        //// Y축과 Z축은 모델 자체를 회전시켜 방향을 맞춤
        //if (i == 1) SceneComp->SetRotation({ 0, 0, 90 }); // Y-axis
        if (i == 2) SceneComp->SetRotation({ 0, -180, 0 }); // Z-axis

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
    if (!TargetActor) return;

    auto& Mouse = Window.GetMouse();
    auto [MouseX, MouseY] = Mouse.GetPosition();

    // 1. 드래그 시작 감지
    if (Mouse.IsLeftPressed() && !bIsDragging)
    {
        // 마우스 위치에서 3D 월드 공간으로 쏘는 광선을 생성합니다.
        auto CameraSceneComp = USceneManager::GetInstance().GetMainCameraActor()->GetComponent<USceneComponent>();
        FVector RayOrigin = CameraSceneComp->GetLocation();

        float NDCX = 2.0f * MouseX / Window.GetWidth() - 1.0f;
        float NDCY = 1.0f - 2.0f * MouseY / Window.GetHeight();

        FMatrix InvViewProj = (View * Proj).Inverse();
        FVector4 WorldNear = FVector4(NDCX, NDCY, 0.0f, 1.0f) * InvViewProj;
        WorldNear /= WorldNear.W;

        FVector RayDir = (WorldNear.ToVector3() - RayOrigin).GetNormalized();

        // 이 거리 임계값보다 광선과 축이 가까우면 선택된 것으로 간주합니다 (감도 조절 가능).
        float MinSelectDistance = 0.2f;
        float ClosestDistance = (std::numeric_limits<float>::max)();
        EAxis ClosestAxis = EAxis::None;

        FVector GizmoOrigin = TargetActor->GetComponent<USceneComponent>()->GetLocation();
        FVector AxesDirs[] = { FVector(1,0,0), FVector(0,1,0), FVector(0,0,1) };

        for (int i = 0; i < 3; ++i)
        {
            FVector AxisDir = AxesDirs[i];

            // 두 3D 직선(마우스 광선, 기즈모 축) 사이의 가장 짧은 거리를 계산합니다.
            FVector CrossProduct = RayDir.Cross(AxisDir);
            float Denom = CrossProduct.LengthSquared();

            // 두 직선이 평행한 경우, 점과 직선 사이의 거리로 계산합니다.
            if (Denom < 1e-6f)
            {
                // 이 경우는 현재 구현에서 생략합니다 (일반적으로 발생하기 어려움).
                continue;
            }

            FVector VecBetweenOrigins = GizmoOrigin - RayOrigin;
            float Dist = abs(VecBetweenOrigins.Dot(CrossProduct)) / sqrt(Denom);

            if (Dist < ClosestDistance)
            {
                ClosestDistance = Dist;
                // 가장 가까운 축을 임시로 저장해 둡니다.
                if (Dist < MinSelectDistance)
                {
                    ClosestAxis = static_cast<EAxis>(i + 1);
                }
            }
        }

        if (ClosestAxis != EAxis::None)
        {
            bIsDragging = true;
            ActiveAxis = ClosestAxis;
            DragStartActorLocation = TargetActor->GetComponent<USceneComponent>()->GetLocation();
        }
    }
    // 2. 드래그 종료 감지
    else if (!Mouse.IsLeftPressed() && bIsDragging)
    {
        bIsDragging = false;
        ActiveAxis = EAxis::None;
    }

    // 3. 드래그 중 이동 처리 (이전과 동일)
    if (bIsDragging)
    {
        float MouseDeltaX = static_cast<float>(Mouse.GetXPositionDelta());
        float MouseDeltaY = static_cast<float>(Mouse.GetYPositionDelta());

        if (abs(MouseDeltaX) < 0.1f && abs(MouseDeltaY) < 0.1f) return;

        auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
        auto Camera = USceneManager::GetInstance().GetMainCameraActor();
        auto CameraSceneComp = Camera->GetComponent<USceneComponent>();

        FVector AxisDirection;
        switch (ActiveAxis)
        {
        case EAxis::X: AxisDirection = FVector(1, 0, 0); break;
        case EAxis::Y: AxisDirection = FVector(0, 1, 0); break;
        case EAxis::Z: AxisDirection = FVector(0, 0, 1); break;
        default: return;
        }

        FMatrix VP = View * Proj;
        FVector TargetPos = TargetSceneComp->GetLocation();

        FVector4 ScreenAxisStart_NDC = FVector4(TargetPos, 1.0f) * VP;
        FVector4 ScreenAxisEnd_NDC = FVector4(TargetPos + AxisDirection, 1.0f) * VP;

        ScreenAxisStart_NDC /= ScreenAxisStart_NDC.W;
        ScreenAxisEnd_NDC /= ScreenAxisEnd_NDC.W;

        FVector2D ScreenAxisVec = { ScreenAxisEnd_NDC.X - ScreenAxisStart_NDC.X, ScreenAxisStart_NDC.Y - ScreenAxisEnd_NDC.Y };
        ScreenAxisVec.Normalize();

        FVector2D MouseDeltaVec = { MouseDeltaX, MouseDeltaY };

        float MoveAmount = MouseDeltaVec.Dot(ScreenAxisVec);

        float Distance = (TargetPos - CameraSceneComp->GetLocation()).Length();
        MoveAmount *= Distance * 0.001f;

        TargetSceneComp->TranslateTransform(AxisDirection * MoveAmount);
    }
}
// [추가] GetDepthStencilView 함수의 구현
ID3D11DepthStencilView* URenderer::GetDepthStencilView()
{
    return DepthStencilView.Get();
}