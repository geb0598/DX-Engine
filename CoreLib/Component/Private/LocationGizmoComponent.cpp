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
    float ScaleFactor = Distance * 0.1f; // ũ  
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    AActor* Axes[] = { XAxisActor, YAxisActor, ZAxisActor };
    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = Axes[i]->GetComponent<USceneComponent>();
        auto PrimComp = Axes[i]->GetComponent<UStaticMeshComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());
        //// Y Z  ü ȸ  
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

    auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
    auto CameraSceneComp = CameraActor->GetComponent<USceneComponent>();

    // 1. 드래그 시작 처리
    if (Mouse.IsLeftPressed() && !bIsDragging)
    {
        // (기존과 동일한 축 선택 로직)
        float MinSelectDistance = 0.2f;
        float ClosestDistance = (std::numeric_limits<float>::max)();
        EAxis ClosestAxis = EAxis::None;

        FVector RayOrigin = CameraSceneComp->GetLocation();
        float NDCX = 2.0f * MouseX / Window.GetWidth() - 1.0f;
        float NDCY = 1.0f - 2.0f * MouseY / Window.GetHeight();
        FMatrix InvViewProj = (View * Proj).Inverse();
        FVector4 WorldNear = FVector4(NDCX, NDCY, 0.0f, 1.0f) * InvViewProj;
        WorldNear /= WorldNear.W;
        FVector RayDir = (WorldNear.ToVector3() - RayOrigin).GetNormalized();

        FVector GizmoOrigin = TargetActor->GetComponent<USceneComponent>()->GetLocation();
        FVector AxesDirs[] = { FVector(1,0,0), FVector(0,1,0), FVector(0,0,1) };

        for (int i = 0; i < 3; ++i)
        {
            FVector AxisDir = AxesDirs[i];
            FVector CrossProduct = RayDir.Cross(AxisDir);
            float Denom = CrossProduct.LengthSquared();
            if (Denom < 1e-6f) continue;
            float Dist = abs((GizmoOrigin - RayOrigin).Dot(CrossProduct)) / sqrt(Denom);
            if (Dist < ClosestDistance)
            {
                ClosestDistance = Dist;
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
            auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
            DragStartActorLocation = TargetSceneComp->GetLocation();

            // [수정] 드래그 평면 생성 및 시작점 계산
            // 뷰 행렬의 세 번째 열이 카메라의 Forward 벡터입니다. (행렬이 Transpose 되어있으므로)
            DragPlaneNormal = FVector(View.M[0][2], View.M[1][2], View.M[2][2]);

            float Denominator = RayDir.Dot(DragPlaneNormal);
            if (abs(Denominator) > 1e-6f)
            {
                float t = (DragStartActorLocation - RayOrigin).Dot(DragPlaneNormal) / Denominator;
                DragStartPoint_World = RayOrigin + RayDir * t;
            }
        }
    }
    // 2. 드래그 종료 처리
    else if (!Mouse.IsLeftPressed() && bIsDragging)
    {
        bIsDragging = false;
        ActiveAxis = EAxis::None;
    }

    // 3. 드래그 중 이동 처리
    if (bIsDragging)
    {
        FVector RayOrigin = CameraSceneComp->GetLocation();
        float NDCX = 2.0f * MouseX / Window.GetWidth() - 1.0f;
        float NDCY = 1.0f - 2.0f * MouseY / Window.GetHeight();
        FMatrix InvViewProj = (View * Proj).Inverse();
        FVector4 WorldNear = FVector4(NDCX, NDCY, 0.0f, 1.0f) * InvViewProj;
        WorldNear /= WorldNear.W;
        FVector RayDir = (WorldNear.ToVector3() - RayOrigin).GetNormalized();

        float Denominator = RayDir.Dot(DragPlaneNormal);
        if (abs(Denominator) > 1e-6f)
        {
            // 현재 마우스 위치의 광선이 드래그 평면과 만나는 점을 계산
            float t = (DragStartActorLocation - RayOrigin).Dot(DragPlaneNormal) / Denominator;
            FVector CurrentWorldPoint = RayOrigin + RayDir * t;

            // 드래그 시작점부터 현재점까지의 월드 공간에서의 이동 벡터
            FVector WorldDelta = CurrentWorldPoint - DragStartPoint_World;

            // 선택된 축의 방향 벡터
            FVector AxisDirection;
            switch (ActiveAxis)
            {
            case EAxis::X: AxisDirection = FVector(1, 0, 0); break;
            case EAxis::Y: AxisDirection = FVector(0, 1, 0); break;
            case EAxis::Z: AxisDirection = FVector(0, 0, 1); break;
            default: return;
            }

            // 월드 이동 벡터를 선택된 축에 투영하여 축 방향으로의 이동량만 추출
            FVector ProjectedDelta = AxisDirection * WorldDelta.Dot(AxisDirection);

            // 오브젝트의 새 위치 계산 및 적용
            auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
            TargetSceneComp->SetLocation(DragStartActorLocation + ProjectedDelta);
        }
    }
}