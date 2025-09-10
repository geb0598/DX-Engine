#include "Component/Public/ScaleGizmoComponent.h"
#include "Component/Public/StaticMeshComponent.h"
#include "Mesh/Public/GizmoShapes.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "RayCaster/Raycaster.h"
#include "Utilities/Utilities.h"
#include <limits>
#include <algorithm>

UScaleGizmoComponent::UScaleGizmoComponent(AActor* Actor)
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
    for (const auto& v : XAxisScaleVertices) { VerticesX.push_back(static_cast<FVertex>(v)); }
    auto XMesh = std::make_shared<UMesh>(Device, VerticesX);
    XAxisActor->AddComponent<UStaticMeshComponent>(XAxisActor, GizmoVertexShader, GizmoPixelShader, XMesh);
    XAxisActor->AddComponent<USceneComponent>(XAxisActor);

    // Y-Axis Actor
    YAxisActor = new AActor();
    TArray<FVertex> VerticesY;
    for (const auto& v : YAxisScaleVertices) { VerticesY.push_back(static_cast<FVertex>(v)); }
    auto YMesh = std::make_shared<UMesh>(Device, VerticesY);
    YAxisActor->AddComponent<UStaticMeshComponent>(YAxisActor, GizmoVertexShader, GizmoPixelShader, YMesh);
    YAxisActor->AddComponent<USceneComponent>(YAxisActor);

    // Z-Axis Actor
    ZAxisActor = new AActor();
    TArray<FVertex> VerticesZ;
    for (const auto& v : ZAxisScaleVertices) { VerticesZ.push_back(static_cast<FVertex>(v)); }
    auto ZMesh = std::make_shared<UMesh>(Device, VerticesZ);
    ZAxisActor->AddComponent<UStaticMeshComponent>(ZAxisActor, GizmoVertexShader, GizmoPixelShader, ZMesh);
    ZAxisActor->AddComponent<USceneComponent>(ZAxisActor);
}

UScaleGizmoComponent::~UScaleGizmoComponent()
{
}

void UScaleGizmoComponent::Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj)
{
    if (!TargetActor) return;

    auto* DeviceContext = Renderer.GetDeviceContext();
    auto* DepthStencilView = Renderer.GetDepthStencilView();

    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
    SetLocation(TargetSceneComp->GetLocation());

    auto CameraLocation = USceneManager::GetInstance().GetMainCameraActor()->GetComponent<USceneComponent>()->GetLocation();
    float Distance = (GetLocation() - CameraLocation).Length();
    float ScaleFactor = Distance * 0.1f;
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    AActor* Axes[] = { XAxisActor, YAxisActor, ZAxisActor };
    FVector CameraForward = (GetLocation() - CameraLocation).GetNormalized();

    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = Axes[i]->GetComponent<USceneComponent>();
        auto PrimComp = Axes[i]->GetComponent<UStaticMeshComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());

        FMatrix ModelMatrix;
        if (i == 2) // Z축 (파란색)일 경우에만 특별 처리
        {
            // 화살표 모델을 X축 기준으로 180도 회전시켜 방향을 뒤집습니다.
            const float PI = 3.1415926535f;
            FMatrix RotationMatrix = FMatrix::CreateRotationX(PI);

            // 기본 모델 행렬에 추가로 회전 변환을 곱해줍니다.
            ModelMatrix = RotationMatrix * SceneComp->GetModelingMatrix();
        }
        else // X, Y 축은 기존 방식 그대로 사용
        {
            ModelMatrix = SceneComp->GetModelingMatrix();
        }

        FMatrix MVP = ModelMatrix * View * Proj;

        int bIsSelected = (static_cast<int>(ActiveAxis) == i + 1) ? 1 : 0;
        PrimComp->GetVertexShader()->UpdateConstantBuffer(DeviceContext, "constants", &MVP);
        PrimComp->GetPixelShader()->UpdateConstantBuffer(DeviceContext, "constants", &bIsSelected);

        PrimComp->GetVertexShader()->Bind(DeviceContext, "constants");
        PrimComp->GetPixelShader()->Bind(DeviceContext, "constants");

        PrimComp->Render(DeviceContext);
    }
}

void UScaleGizmoComponent::HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj)
{
    if (!TargetActor) return;

    auto& Mouse = Window.GetMouse();
    auto [MouseX, MouseY] = Mouse.GetPosition();

    auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
    auto CameraSceneComp = CameraActor->GetComponent<USceneComponent>();
    FVector RayOrigin = CameraSceneComp->GetLocation();
    FVector GizmoOrigin = TargetActor->GetComponent<USceneComponent>()->GetLocation();

    // 1. 드래그 시작 처리
    if (Mouse.IsLeftPressed() && !bIsDragging)
    {
        // Z축은 카메라 방향을 기준으로 동적으로 계산
        FVector CameraForward = (GizmoOrigin - RayOrigin).GetNormalized();
        FVector AxesDirs[] = { FVector(1,0,0), FVector(0,1,0), FVector(0,0,-1) };
        EAxis ClosestAxis = EAxis::None;

        float ClosestDistance2D = (std::numeric_limits<float>::max)();
        const float MinPixelDistance = 10.0f;

        FMatrix VP = View * Proj;
        float ScaleFactor = (GizmoOrigin - RayOrigin).Length() * 0.1f;

        for (int i = 0; i < 3; ++i)
        {
            FVector AxisDir = AxesDirs[i];

            FVector AxisStart_World = GizmoOrigin;
            FVector AxisEnd_World = GizmoOrigin + (i == 2 ? FVector(0,0,-1) : AxisDir) * 1.0f * ScaleFactor;
            
            FVector4 AxisStart_Clip = FVector4(AxisStart_World, 1.0f) * VP;
            FVector4 AxisEnd_Clip = FVector4(AxisEnd_World, 1.0f) * VP;

            if (AxisStart_Clip.W <= 0 || AxisEnd_Clip.W <= 0) continue;

            AxisStart_Clip /= AxisStart_Clip.W;
            AxisEnd_Clip /= AxisEnd_Clip.W;

            FVector2D AxisStart_Screen((AxisStart_Clip.X + 1.0f) * 0.5f * Window.GetWidth(), (1.0f - AxisStart_Clip.Y) * 0.5f * Window.GetHeight());
            FVector2D AxisEnd_Screen((AxisEnd_Clip.X + 1.0f) * 0.5f * Window.GetWidth(), (1.0f - AxisEnd_Clip.Y) * 0.5f * Window.GetHeight());

            FVector2D MousePos(static_cast<float>(MouseX), static_cast<float>(MouseY));
            FVector2D LineVec = AxisEnd_Screen - AxisStart_Screen;
            FVector2D PointVec = MousePos - AxisStart_Screen;
            
            float LineLenSq = LineVec.LengthSquared();
            float t = 0.0f;
            if (LineLenSq > 1e-6f)
            {
                t = PointVec.Dot(LineVec) / LineLenSq;
                t = (std::max)(0.0f, (std::min)(1.0f, t));
            }

            FVector2D ClosestPointOnLine = AxisStart_Screen + LineVec * t;
            float Dist2D = (MousePos - ClosestPointOnLine).Length();

            if (Dist2D < ClosestDistance2D)
            {
                ClosestDistance2D = Dist2D;
                if (Dist2D < MinPixelDistance)
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
            DragStartActorScale = TargetSceneComp->GetScale();

            float NDCX = 2.0f * MouseX / Window.GetWidth() - 1.0f;
            float NDCY = 1.0f - 2.0f * MouseY / Window.GetHeight();
            FMatrix InvViewProj = (View * Proj).Inverse();
            FVector4 WorldNear = FVector4(NDCX, NDCY, 0.0f, 1.0f) * InvViewProj;
            WorldNear /= WorldNear.W;
            FVector RayDir = (WorldNear.ToVector3() - RayOrigin).GetNormalized();

            DragPlaneNormal = FVector(View.M[0][2], View.M[1][2], View.M[2][2]);

            float Denominator = RayDir.Dot(DragPlaneNormal);
            if (abs(Denominator) > 1e-6f)
            {
                float t = (DragStartActorScale - RayOrigin).Dot(DragPlaneNormal) / Denominator;
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

    // 3. 드래그 중 스케일 처리
    if (bIsDragging)
    {
        float NDCX = 2.0f * MouseX / Window.GetWidth() - 1.0f;
        float NDCY = 1.0f - 2.0f * MouseY / Window.GetHeight();
        FMatrix InvViewProj = (View * Proj).Inverse();
        FVector4 WorldNear = FVector4(NDCX, NDCY, 0.0f, 1.0f) * InvViewProj;
        WorldNear /= WorldNear.W;
        FVector RayDir = (WorldNear.ToVector3() - RayOrigin).GetNormalized();

        float Denominator = RayDir.Dot(DragPlaneNormal);
        if (abs(Denominator) > 1e-6f)
        {
            float t = (DragStartActorScale - RayOrigin).Dot(DragPlaneNormal) / Denominator;
            FVector CurrentWorldPoint = RayOrigin + RayDir * t;
            FVector WorldDelta = CurrentWorldPoint - DragStartPoint_World;

            FVector AxisDirection;
            switch (ActiveAxis)
            {
            case EAxis::X: AxisDirection = FVector(1, 0, 0); break;
            case EAxis::Y: AxisDirection = FVector(0, 1, 0); break;
            case EAxis::Z: AxisDirection = FVector(0, 0, 1); break;
            default: return;
            }

            float ProjectedDistance = WorldDelta.Dot(AxisDirection);

            if (ActiveAxis == EAxis::Z)
            {
                ProjectedDistance *= -1.0f;
            }

            float ScaleFactor = 1.0f + ProjectedDistance * 0.5f;
            
            auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
            FVector NewScale = DragStartActorScale;
            
            switch (ActiveAxis)
            {
            case EAxis::X:
                NewScale.X = DragStartActorScale.X * ScaleFactor;
                break;
            case EAxis::Y:
                NewScale.Y = DragStartActorScale.Y * ScaleFactor;
                break;
            case EAxis::Z:
                NewScale.Z = DragStartActorScale.Z * ScaleFactor;
                break;
            }
            
            TargetSceneComp->SetScale(NewScale);
        }
    }
}