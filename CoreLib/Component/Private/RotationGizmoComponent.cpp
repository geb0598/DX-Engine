#include "Component/Public/RotationGizmoComponent.h"
#include "Component/Public/StaticMeshComponent.h"
#include "Mesh/Public/GizmoShapes.h"
#include "Renderer/Public/Renderer.h"
#include "Actor/Public/Actor.h"
#include "Shader/Shader.h"
#include "Mesh/Mesh.h"
#include "Scene/Scene.h"
#include "Window/Window.h"
#include "RayCaster/Public/URayCaster.h"
#include <limits>
#include <algorithm>
#include "RayCaster/Raycaster.h"

URotationGizmoComponent::URotationGizmoComponent(AActor* Actor)
    : UGizmoComponent(Actor)
{
    CreateGizmoActors(URenderer::GetInstance());
}

// [추가] 소멸자의 실제 구현. 여기서 메모리를 해제합니다.
URotationGizmoComponent::~URotationGizmoComponent()
{
    delete RingActorX;
    delete RingActorY;
    delete RingActorZ;
}

void URotationGizmoComponent::HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj)
{
    if (!TargetActor) return;

    auto& Mouse = Window.GetMouse();
    auto [MouseX, MouseY] = Mouse.GetPosition();

    auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
    FVector RayOrigin = CameraActor->GetComponent<USceneComponent>()->GetLocation();
    FVector GizmoOrigin = TargetActor->GetComponent<USceneComponent>()->GetLocation();

    FVector RayDir = RayCaster.CurrentRay.Vector;

    // 드래그 종료 처리
    if (!Mouse.IsLeftPressed() && bIsDragging)
    {
        bIsDragging = false;
        ActiveAxis = EAxis::None;
    }

    
    // 드래그 시작 처리
    if (Mouse.IsLeftPressed() && !bIsDragging)
    {
        // [NEW] 가장 가까운 축/거리/히트지점 월드 좌표를 추적
        float ClosestWorldDist = (std::numeric_limits<float>::max)();
        EAxis NewActiveAxis = EAxis::None;
        FVector HitPointWorld = FVector::Zero;

        // [NEW] 렌더와 동일한 스케일(카메라 거리 비례) 계산  :contentReference[oaicite:3]{index=3}
        auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
        FVector CameraLocation = CameraActor->GetComponent<USceneComponent>()->GetLocation();
        float Distance = (GetLocation() - CameraLocation).Length();
        float ScaleFactor = Distance * 0.1f;

        // [NEW] 축별 회전 행렬 (렌더와 동일)  :contentReference[oaicite:4]{index=4}
        const float PI_HALF = 1.57079632679f; // 90도
        FMatrix AxisRotation[3] = {
            FMatrix::CreateRotationY(PI_HALF), // X 링: Y축 90°
            FMatrix::CreateRotationX(PI_HALF), // Y 링: X축 90°
            FMatrix::Identity                  // Z 링: 추가 회전 없음
        };

        // [NEW] 축별로 레이를 '로컬'로 세팅하고 토러스 교차 검사
        for (int i = 0; i < 3; ++i)
        {
            // [NEW] 렌더와 동일한 TRS(ModelMatrix = S * R * T)  :contentReference[oaicite:5]{index=5}
            FMatrix S = FMatrix::CreateScale({ ScaleFactor, ScaleFactor, ScaleFactor });
            FMatrix R = AxisRotation[i];
            FMatrix T = FMatrix::CreateTranslation(GetLocation());
            FMatrix ModelMatrix = S * R * T;

            // [NEW] 이 축의 로컬공간으로 레이 보냄 (M*V*P)^-1 로 역투영됨)  :contentReference[oaicite:6]{index=6}
            RayCaster.SetRayWithMouseAndMVP(
                MouseX, MouseY, Window.GetWidth(), Window.GetHeight(),
                ModelMatrix, View, Proj
            );

            // [NEW] 토러스 근사 피킹 t 구하기  :contentReference[oaicite:7]{index=7}
            float tLocal = RayCaster.RayCastToAnalogousTorus();

            // [NEW] 교차 없으면 스킵 (DONT_INTERSECT = -1.0f 관례)
            if (tLocal < 0.0f)
                continue;

            // [NEW] 월드 거리로 변환하여 가장 가까운 축 선택  :contentReference[oaicite:8]{index=8}
            std::optional<float> distOpt = RayCaster.GetRealWorldDistance(std::optional<float>{ tLocal });
            if (!distOpt)
                continue;

            if (*distOpt < ClosestWorldDist)
            {
                ClosestWorldDist = *distOpt;
                NewActiveAxis = static_cast<EAxis>(i + 1);

                // [NEW] 로컬 히트점 → 월드 히트점 계산 (수학 라이브러리 관례에 맞춰 v * M)  :contentReference[oaicite:9]{index=9}
                const auto& ray = RayCaster.CurrentRay; // SetRay...가 채움
                FVector localHit = ray.Point + tLocal * ray.Vector;
                HitPointWorld = localHit * ModelMatrix;
            }
        }

        // [NEW] 최종 결정 반영
        if (NewActiveAxis != EAxis::None)
        {
            bIsDragging = true;
            ActiveAxis = NewActiveAxis;
            DragStartPoint_World = HitPointWorld; // ★ 토러스 실제 히트 위치 저장
            DragStartActorRotation = TargetActor->GetComponent<USceneComponent>()->GetRotation();
        }
    }


    // 드래그 중 회전 처리
    if (bIsDragging)
    {
        FVector PlaneNormal;
        switch (ActiveAxis)
        {
        case EAxis::X: PlaneNormal = FVector(1, 0, 0); break;
        case EAxis::Y: PlaneNormal = FVector(0, 1, 0); break;
        case EAxis::Z: PlaneNormal = FVector(0, 0, 1); break;
        default: return;
        }

        float Denominator = RayDir.Dot(PlaneNormal);
        if (abs(Denominator) > 1e-6f)
        {
            float t = (GizmoOrigin - RayOrigin).Dot(PlaneNormal) / Denominator;
            FVector CurrentPoint_World = RayOrigin + RayDir * t;

            FVector StartVec = (DragStartPoint_World - GizmoOrigin).GetNormalized();
            FVector CurrentVec = (CurrentPoint_World - GizmoOrigin).GetNormalized();

            float DotProduct = StartVec.Dot(CurrentVec);
            float Angle = acosf(std::clamp(DotProduct, -1.0f, 1.0f));

            FVector Cross = StartVec.Cross(CurrentVec);
            if (Cross.Dot(PlaneNormal) < 0)
            {
                Angle = -Angle;
            }

            auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
            //FVector NewRotation = DragStartActorRotation;
            FVector NewRotation = FVector::Zero;

            float AngleDegrees = RAD_TO_DEG(Angle);
            switch (ActiveAxis)
            {
            case EAxis::X: NewRotation.X += AngleDegrees; break;
            case EAxis::Y: NewRotation.Y += AngleDegrees; break;
            case EAxis::Z: NewRotation.Z += AngleDegrees; break;
            }
            TargetSceneComp->SetRotation(NewRotation);
        }
    }
}

void URotationGizmoComponent::Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj)
{
    // 1. 타겟 액터가 없으면 아무것도 그리지 않습니다.
    if (!TargetActor) return;

    auto* DeviceContext = Renderer.GetDeviceContext();
    auto* DepthStencilView = Renderer.GetDepthStencilView();

    // 기즈모가 다른 물체에 가려지지 않도록 뎁스 버퍼를 초기화합니다.
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // 2. 기즈모의 위치와 크기를 설정합니다.
    auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
    SetLocation(TargetSceneComp->GetLocation()); // 기즈모 위치 = 타겟 액터 위치

 auto CameraLocation = USceneManager::GetInstance().GetMainCameraActor()->GetComponent<USceneComponent>()->GetLocation();    // 카메라와의 거리에 따라 기즈모 크기를 조절합니다.
   
    float Distance = (GetLocation() - CameraLocation).Length();
    float ScaleFactor = Distance * 0.1f;
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    // 3. 각 링(액터)에 대해 렌더링 작업을 반복합니다.
    AActor* RingActors[] = { RingActorX, RingActorY, RingActorZ };
    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = RingActors[i]->GetComponent<USceneComponent>();
        auto PrimComp = RingActors[i]->GetComponent<UStaticMeshComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());

        // 4. 각 축에 맞는 회전 행렬을 계산합니다.
        FMatrix RotationMatrix; // 단위 행렬로 초기화
        const float PI_HALF = 1.57079632679f; // 90도

        if (i == 0) // X축 링 (빨간색) -> Y축으로 90도 회전하여 YZ 평면에 놓습니다.
        {
            RotationMatrix = FMatrix::CreateRotationY(PI_HALF);
        }
        else if (i == 1) // Y축 링 (초록색) -> X축으로 90도 회전하여 XZ 평면에 놓습니다.
        {
            RotationMatrix = FMatrix::CreateRotationX(PI_HALF);
        }
        // Z축 링은 기본적으로 XY 평면에 생성되므로 추가 회전이 필요 없습니다.

        // 5. 최종 모델링 행렬(TRS)을 계산합니다.
        FMatrix ScaleMatrix = FMatrix::CreateScale(GetScale());
        FMatrix TranslationMatrix = FMatrix::CreateTranslation(GetLocation());
        FMatrix ModelMatrix = ScaleMatrix * RotationMatrix * TranslationMatrix;

        FMatrix MVP = ModelMatrix * View * Proj;

        // 6. 셰이더에 데이터를 업데이트하고 렌더링을 호출합니다.
        int bIsSelected = (static_cast<int>(ActiveAxis) == i + 1) ? 1 : 0;
        PrimComp->GetVertexShader()->UpdateConstantBuffer(DeviceContext, "constants", &MVP);
        PrimComp->GetPixelShader()->UpdateConstantBuffer(DeviceContext, "constants", &bIsSelected);

        PrimComp->GetVertexShader()->Bind(DeviceContext, "constants");
        PrimComp->GetPixelShader()->Bind(DeviceContext, "constants");

        PrimComp->Render(DeviceContext);
    }
}

void URotationGizmoComponent::CreateGizmoActors(URenderer& Renderer)
{
    ID3D11Device* Device = Renderer.GetDevice();

    TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc(std::begin(FVertex::InputLayoutDesc), std::end(FVertex::InputLayoutDesc));
    auto GizmoVertexShader = std::make_shared<UVertexShader>(Device, "./Shader/VertexShader.hlsl", "main", LayoutDesc);
    auto GizmoPixelShader = std::make_shared<UPixelShader>(Device, "./Shader/PixelShader.hlsl", "main");

    // [수정] axis 파라미터 없이 색상만 전달하여 호출합니다.
    std::vector<FVertexSimple> TorusVerticesX, TorusVerticesY, TorusVerticesZ;
    CreateTorusVertices(TorusVerticesX, 1.0f, 0.025f, 64, 16, FVector(1, 0, 0)); // Red
    CreateTorusVertices(TorusVerticesY, 1.0f, 0.025f, 64, 16, FVector(0, 1, 0)); // Green
    CreateTorusVertices(TorusVerticesZ, 1.0f, 0.025f, 64, 16, FVector(0, 0, 1)); // Blue

    // X-Axis Ring Actor
    TArray<FVertex> VerticesX;
    for (const auto& v : TorusVerticesX) { VerticesX.push_back(static_cast<FVertex>(v)); }
    auto XMesh = std::make_shared<UMesh>(Device, VerticesX);
    RingActorX = new AActor();
    RingActorX->AddComponent<UStaticMeshComponent>(RingActorX, GizmoVertexShader, GizmoPixelShader, XMesh);
    RotationMeshX = RingActorX->GetComponent<UStaticMeshComponent>();
    RingActorX->AddComponent<USceneComponent>(RingActorX);

    // Y-Axis Ring Actor
    TArray<FVertex> VerticesY;
    for (const auto& v : TorusVerticesY) { VerticesY.push_back(static_cast<FVertex>(v)); }
    auto YMesh = std::make_shared<UMesh>(Device, VerticesY);
    RingActorY = new AActor();
    RingActorY->AddComponent<UStaticMeshComponent>(RingActorY, GizmoVertexShader, GizmoPixelShader, YMesh);
    RotationMeshY = RingActorY->GetComponent<UStaticMeshComponent>();
    RingActorY->AddComponent<USceneComponent>(RingActorY);

    // Z-Axis Ring Actor
    TArray<FVertex> VerticesZ;
    for (const auto& v : TorusVerticesZ) { VerticesZ.push_back(static_cast<FVertex>(v)); }
    auto ZMesh = std::make_shared<UMesh>(Device, VerticesZ);
    RingActorZ = new AActor();
    RingActorZ->AddComponent<UStaticMeshComponent>(RingActorZ, GizmoVertexShader, GizmoPixelShader, ZMesh);
    RotationMeshZ = RingActorZ->GetComponent<UStaticMeshComponent>();
    RingActorZ->AddComponent<USceneComponent>(RingActorZ);
}