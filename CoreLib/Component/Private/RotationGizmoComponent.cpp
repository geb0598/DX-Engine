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

URotationGizmoComponent::URotationGizmoComponent(AActor* Actor)
    : UGizmoComponent(Actor)
{
    CreateGizmoActors(URenderer::GetInstance());
}

URotationGizmoComponent::~URotationGizmoComponent()
{
}

void URotationGizmoComponent::HandleInput(URayCaster& RayCaster, UWindow& Window, const FMatrix& View, const FMatrix& Proj)
{
    if (!TargetActor) return;

    auto& Mouse = Window.GetMouse();
    auto [MouseX, MouseY] = Mouse.GetPosition();

    auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
    FVector RayOrigin = CameraActor->GetComponent<USceneComponent>()->GetLocation();
    FVector GizmoOrigin = TargetActor->GetComponent<USceneComponent>()->GetLocation();

    RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, Window.GetWidth(), Window.GetHeight(), FMatrix::Identity, View, Proj);
    FVector RayDir = RayCaster.CurrentRay.Vector;

    if (!Mouse.IsLeftPressed() && bIsDragging)
    {
        bIsDragging = false;
        ActiveAxis = EAxis::None;
    }

    if (Mouse.IsLeftPressed() && !bIsDragging)
    {
        float ClosestHitDistance = (std::numeric_limits<float>::max)();
        EAxis NewActiveAxis = EAxis::None;
        FVector HitPoint;

        FVector PlaneNormals[] = { FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1) };
        float GizmoRadius = (GetLocation() - CameraActor->GetComponent<USceneComponent>()->GetLocation()).Length() * 0.1f;

        for (int i = 0; i < 3; ++i)
        {
            FVector PlaneNormal = PlaneNormals[i];
            float Denominator = RayDir.Dot(PlaneNormal);
            if (abs(Denominator) > 1e-6f)
            {
                float t = (GizmoOrigin - RayOrigin).Dot(PlaneNormal) / Denominator;
                if (t > 0)
                {
                    FVector IntersectionPoint = RayOrigin + RayDir * t;
                    float DistanceFromOrigin = (IntersectionPoint - GizmoOrigin).Length();

                    if (abs(DistanceFromOrigin - GizmoRadius) < 0.05f * GizmoRadius)
                    {
                        if (t < ClosestHitDistance)
                        {
                            ClosestHitDistance = t;
                            NewActiveAxis = static_cast<EAxis>(i + 1);
                            HitPoint = IntersectionPoint;
                        }
                    }
                }
            }
        }

        if (NewActiveAxis != EAxis::None)
        {
            bIsDragging = true;
            ActiveAxis = NewActiveAxis;
            DragStartPoint_World = HitPoint;
            DragStartActorRotation = TargetActor->GetComponent<USceneComponent>()->GetRotation();
            DragStartMouseX = MouseX;
            DragStartMouseY = MouseY;
        }
    }

    if (bIsDragging)
    {
        float TotalDeltaX = MouseX - DragStartMouseX;
        float TotalDeltaY = MouseY - DragStartMouseY;
        
        float RotationSpeed = 0.5f;
        float AngleDelta = 0.0f;
        
        switch (ActiveAxis)
        {
        case EAxis::X: 
            AngleDelta = -TotalDeltaY * RotationSpeed; 
            break;
        case EAxis::Y: 
            AngleDelta = TotalDeltaX * RotationSpeed; 
            break;
        case EAxis::Z: 
            AngleDelta = TotalDeltaX * RotationSpeed; 
            break;
        default: return;
        }
        
        auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
        FVector NewRotation = DragStartActorRotation;
        
        switch (ActiveAxis)
        {
        case EAxis::X: NewRotation.X += AngleDelta; break;
        case EAxis::Y: NewRotation.Y += AngleDelta; break;
        case EAxis::Z: NewRotation.Z += AngleDelta; break;
        }
        TargetSceneComp->SetRotation(NewRotation);
    }
}

void URotationGizmoComponent::Render(URenderer& Renderer, const FMatrix& View, const FMatrix& Proj)
{
    // 1. Ÿ�� ���Ͱ� ������ �ƹ��͵� �׸��� �ʽ��ϴ�.
    if (!TargetActor) return;

    auto* DeviceContext = Renderer.GetDeviceContext();
    auto* DepthStencilView = Renderer.GetDepthStencilView();

    // ����� �ٸ� ��ü�� �������� �ʵ��� ���� ���۸� �ʱ�ȭ�մϴ�.
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // 2. ������� ��ġ�� ũ�⸦ �����մϴ�.
    auto TargetSceneComp = TargetActor->GetComponent<USceneComponent>();
    SetLocation(TargetSceneComp->GetLocation()); // ����� ��ġ = Ÿ�� ���� ��ġ

 auto CameraLocation = USceneManager::GetInstance().GetMainCameraActor()->GetComponent<USceneComponent>()->GetLocation();    // ī�޶���� �Ÿ��� ���� ����� ũ�⸦ �����մϴ�.
   
    float Distance = (GetLocation() - CameraLocation).Length();
    float ScaleFactor = Distance * 0.1f;
    SetScale({ ScaleFactor, ScaleFactor, ScaleFactor });

    // 3. �� ��(����)�� ���� ������ �۾��� �ݺ��մϴ�.
    AActor* RingActors[] = { RingActorX, RingActorY, RingActorZ };
    for (int i = 0; i < 3; ++i)
    {
        auto SceneComp = RingActors[i]->GetComponent<USceneComponent>();
        auto PrimComp = RingActors[i]->GetComponent<UStaticMeshComponent>();

        SceneComp->SetLocation(GetLocation());
        SceneComp->SetScale(GetScale());

        // 4. �� �࿡ �´� ȸ�� ����� ����մϴ�.
        FMatrix RotationMatrix; // ���� ��ķ� �ʱ�ȭ
        const float PI_HALF = 1.57079632679f; // 90��

        if (i == 0) // X�� �� (������) -> Y������ 90�� ȸ���Ͽ� YZ ��鿡 �����ϴ�.
        {
            RotationMatrix = FMatrix::CreateRotationY(PI_HALF);
        }
        else if (i == 1) // Y�� �� (�ʷϻ�) -> X������ 90�� ȸ���Ͽ� XZ ��鿡 �����ϴ�.
        {
            RotationMatrix = FMatrix::CreateRotationX(PI_HALF);
        }
        // Z�� ���� �⺻������ XY ��鿡 �����ǹǷ� �߰� ȸ���� �ʿ� �����ϴ�.

        // 5. ���� �𵨸� ���(TRS)�� ����մϴ�.
        FMatrix ScaleMatrix = FMatrix::CreateScale(GetScale());
        FMatrix TranslationMatrix = FMatrix::CreateTranslation(GetLocation());
        FMatrix ModelMatrix = ScaleMatrix * RotationMatrix * TranslationMatrix;

        FMatrix MVP = ModelMatrix * View * Proj;

        // 6. ���̴��� �����͸� ������Ʈ�ϰ� �������� ȣ���մϴ�.
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

    // [����] axis �Ķ���� ���� ���� �����Ͽ� ȣ���մϴ�.
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