#include "pch.h"
#include "CargoComponent.h"

#include "AudioComponent.h"
#include "PhysScene.h"
#include "PlayerCameraManager.h"
#include "StaticMeshComponent.h"
#include "Vehicle.h"

// ====================================================================
// 생성자 & 소멸자
// ====================================================================

UCargoComponent::UCargoComponent()
{
    bCanEverTick = true;

    CargoMeshOptions.Add("Data/Model/Box/cardboard-box/box-1.obj");
    CargoMeshOptions.Add("Data/Model/Box/cardboard-box/box-2.obj");
    CargoMeshOptions.Add("Data/Model/Box/cardboard-box/box-3.obj");
}

UCargoComponent::~UCargoComponent()
{
}

// ====================================================================
// 언리얼 엔진 생명주기 (Lifecycle)
// ====================================================================

void UCargoComponent::InitializeComponent()
{
    UActorComponent::InitializeComponent();
}

void UCargoComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
}

void UCargoComponent::TickComponent(float DeltaSeconds)
{
    UActorComponent::TickComponent(DeltaSeconds);

    if (CurrentState == ECargoState::Collapsed || CargoBoxes.IsEmpty())
    {
        return;
    }

    CheckBoxInteraction();

    if (CurrentState == ECargoState::Collapsed)
    {
        return;
    }

    FTransform CompTransform = GetWorldTransform();

    FVector WorldGravityDir = FVector(0, 0, -1);

    FTransform InvTransform = CompTransform.Inverse();
    FVector LocalGravityDir = InvTransform.TransformVector(WorldGravityDir);

    float DotUp = -LocalGravityDir.Z; 
    float LeanAngle = RadiansToDegrees(std::acos(FMath::Clamp(DotUp, -1.0f, 1.0f)));

    // ===== 카메라 연출 =====
    APlayerCameraManager* PlayerCameraManager = GetWorld()->GetPlayerCameraManager();
    if (PlayerCameraManager)
    {
        float DangerRatio = FMath::Clamp(LeanAngle / CriticalAngle, 0.0f, 1.0f);

        float TargetIntensity = FMath::Lerp(0.0f, 2.0f, DangerRatio);
        float TargetRadius    = FMath::Lerp(0.6f, 0.2f, DangerRatio);
        float TargetSoftness  = 0.5f; 

        PlayerCameraManager->AdjustVignette(
            -1.0f,                                      // Duration
            TargetRadius,                               // Radius (작을수록 화면 중앙까지 침범)
            TargetSoftness,                             // Softness (경계 흐림 정도)
            TargetIntensity,                            // Intensity (어두운 정도)
            2.0f,                                       // Roundness (원형)
            FLinearColor(0.6f, 0.0f, 0.0f, 0.8f), // Color
            10                                          // Priority (높은 우선순위)
        );
    }
    // =====================

    if (LeanAngle > CriticalAngle)
    {
        CollapseAll();
        return;
    }

    float PitchInput = LocalGravityDir.X;
    float RollInput = LocalGravityDir.Y;

    PitchInput *= PitchSensitivity;
    RollInput *= RollSensitivity;

    switch (SwayMode)
    {
    case ECargoSwayMode::RollOnly:
        PitchInput = 0.0f;
        break;
    case ECargoSwayMode::PitchOnly:
        RollInput = 0.0f;
        break;
    case ECargoSwayMode::Locked:
        PitchInput = 0.0f;
        RollInput = 0.0f;
        break;
    }
    
    FVector LeanDirection = FVector(PitchInput, RollInput, 0.0f);
    
    float LeanMagnitude = LeanDirection.Size();
    if (LeanMagnitude > 0.001f)
    {
        LeanDirection /= LeanMagnitude;
    }
    else
    {
        LeanDirection = FVector(0, 0, 0);
    }

    bool bIsCollapseTriggered = false;
    int32 FirstCollapseIndex = -1;

    for (int32 i = 1; i < ValidCargoCount; i++)
    {
        UStaticMeshComponent* CurrentCargo = CargoBoxes[i];
        UStaticMeshComponent* ParentCargo = CargoBoxes[i - 1];
        if (!CurrentCargo || !ParentCargo)
        {
            continue;
        }

        // @note 위층으로 갈수록 밀리는 정도를 선형 증가시킴
        float LayerWeight = static_cast<float>(i);

        FVector TargetOffset = LeanDirection * (LeanMagnitude * BaseSlideAmount * LayerWeight);

        FVector CurrentLocation = CurrentCargo->GetRelativeLocation();

        FVector TargetLocation = FVector(TargetOffset.X, TargetOffset.Y, CurrentLocation.Z);

        // @note 기울기가 클수록 화물이 더 빠르게 이동
        float LeanSpeed = FMath::Lerp(MinSwaySpeed, MaxSwaySpeed, LeanMagnitude);
        FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaSeconds, LeanSpeed);

        CurrentCargo->SetRelativeLocation(NewLocation);

        if (CheckCollapseRelative(ParentCargo, NewLocation))
        {
            bIsCollapseTriggered = true;
            FirstCollapseIndex = i;
            break;
        }
    }
    
    if (bIsCollapseTriggered)
    {
        CurrentDangerDuration += DeltaSeconds;
        
        if (CurrentDangerDuration > CollapseGraceTime)
        {
            CollapseFrom(FirstCollapseIndex);
            CurrentDangerDuration = 0.0f; 
        }
    }
    else
    {
        CurrentDangerDuration = FMath::Max(0.0f, CurrentDangerDuration - DeltaSeconds * 2.0f);
    }
}

void UCargoComponent::OnRegister(UWorld* InWorld)
{
    UActorComponent::OnRegister(InWorld);

    // @todo 디버그용 (테스트 후 삭제할 것)
    InitializeCargo(20);
}

void UCargoComponent::OnUnregister()
{
    UActorComponent::OnUnregister();

    if (!CargoBoxes.IsEmpty())
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            Owner->RemoveOwnedComponent(CargoBoxes[0]);
        }
        CargoBoxes.Empty();
    }
    ValidCargoCount = 0;
}

// ====================================================================
// 화물 설정 및 초기화
// ====================================================================

int32 UCargoComponent::Lua_ValidCargoCount()
{
    return ValidCargoCount;
}

void UCargoComponent::InitializeCargo(int32 BoxCount)
{
    static std::random_device RandomDevice;
    static std::mt19937 Gen(RandomDevice());
    
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    
    if (!CargoBoxes.IsEmpty())
    {
        if (Owner)
        {
            Owner->RemoveOwnedComponent(CargoBoxes[0]);
        }
        CargoBoxes.Empty();
    }

    USceneComponent* ParentComp = this;
    
    for (int32 i = 0; i < BoxCount; i++)
    {
        UStaticMeshComponent* NewCargo = Cast<UStaticMeshComponent>(Owner->AddNewComponent(UStaticMeshComponent::StaticClass(), ParentComp));
        if (!NewCargo)
        {
            continue;
        }

        FString SelectedMesh = "Data/cube-tex.obj";

        if (!CargoMeshOptions.IsEmpty())
        {
            std::uniform_int_distribution Dist(0, CargoMeshOptions.Num() - 1);
            int32 RandIdx = Dist(Gen);
            SelectedMesh = CargoMeshOptions[RandIdx];
        }

        NewCargo->SetStaticMesh(SelectedMesh);
        NewCargo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewCargo->SetSimulatePhysics(false);

        float NewCargoRotation = 0.0f;
        if (MinCargoRotation <= MaxCargoRotation)
        {
            std::uniform_real_distribution<float> Dist(MinCargoRotation, MaxCargoRotation);
            NewCargoRotation = Dist(Gen);
        }
        NewCargo->SetRelativeRotationEuler(FVector(0, 0, NewCargoRotation));

        float CurrentBottom = 0.0f;
        if (NewCargo->GetStaticMesh())
        {
            FAABB Bound = NewCargo->GetStaticMesh()->GetLocalBound();
            CurrentBottom = Bound.Min.Z;
        }

        float ParentBottom = 0.0f;
        if (i > 0)
        {
            UStaticMeshComponent* ParentMeshComp = Cast<UStaticMeshComponent>(ParentComp);
            if (ParentMeshComp && ParentMeshComp->GetStaticMesh())
            {
                FAABB ParentBound = ParentMeshComp->GetStaticMesh()->GetLocalBound();
                ParentBottom = ParentBound.Max.Z;
            }
        }

        float Offset = ParentBottom - CurrentBottom;
        
        NewCargo->SetRelativeLocation(FVector(0, 0, Offset));

        CargoBoxes.Add(NewCargo);
        ParentComp = NewCargo;
    }

    ValidCargoCount = CargoBoxes.Num();
    CurrentState = ECargoState::Stable;
}

void UCargoComponent::CheckBoxInteraction()
{
AActor* Owner = GetOwner();
    if (!Owner) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FPhysScene* PhysScene = World->GetPhysicsScene();
    if (!PhysScene) return;

    FVector ActorForward = Owner->GetActorRotation().GetForwardVector();

    for (int32 i = 0; i < ValidCargoCount; ++i)
    {
        UStaticMeshComponent* BoxComp = CargoBoxes[i];
        if (!BoxComp || !BoxComp->GetStaticMesh()) continue;

        FTransform BoxTransform = BoxComp->GetWorldTransform();
        FVector BoxLocation = BoxTransform.Translation; // 현재 위치
        FQuat BoxRotation = BoxTransform.Rotation;
        FVector BoxScale = BoxTransform.Scale3D;

        FAABB LocalBound = BoxComp->GetStaticMesh()->GetLocalBound();
        FVector BoxSize = LocalBound.Max - LocalBound.Min;
        FVector HalfExtent = BoxSize * 0.5f * BoxScale;

        HalfExtent *= SweepExtentScale;

        FVector SweepStart = BoxLocation;
        FVector SweepEnd = BoxLocation + ActorForward * 0.1f; 

        FHitResult Hit;
        bool bHit = PhysScene->SweepSingleBox(
            SweepStart,     // 시작
            SweepEnd,       // 끝 (진행 방향으로 살짝 이동)
            HalfExtent,     // 박스 크기
            BoxRotation,    // 회전
            Hit,         // 결과
            Owner           // IgnoreActor: 차량(Owner) 및 그 부속물(화물 포함)은 모두 무시
        );

        if (bHit && Hit.Actor.Get())
        {
            CollapseFrom(i);
            break;
        }
    }
}

// ====================================================================
// 내부 구현 함수
// ====================================================================

bool UCargoComponent::CheckCollapseRelative(UStaticMeshComponent* ParentCargo, const FVector& CurrentLocation)
{
    if (!ParentCargo || !ParentCargo->GetStaticMesh())
    {
        return false;
    }

    FAABB ParentBound = ParentCargo->GetStaticMesh()->GetLocalBound();

    float ParentX = ParentBound.Max.X - ParentBound.Min.X;
    float ParentY = ParentBound.Max.Y - ParentBound.Min.Y;

    float LimitX = ParentX * FalloffThresholdX;
    float LimitY = ParentY * FalloffThresholdY;

    bool bFailX = FMath::Abs(CurrentLocation.X) > LimitX;
    bool bFailY = FMath::Abs(CurrentLocation.Y) > LimitY;

    return (bFailX || bFailY);
}

void UCargoComponent::CollapseFrom(int32 StartIndex)
{
    if (StartIndex < 0 || StartIndex >= ValidCargoCount)
    {
        return;
    }

    
    int32 DropCount = ValidCargoCount - StartIndex;

    // ===== 카메라 연출 =====
    APlayerCameraManager* CameraManager = GetWorld()->GetPlayerCameraManager();
    if (CameraManager && DropCount > 0)
    {
        const float BaseDuration = 0.3f;   // 기본 지속 시간
        const float BaseAmpLoc = 0.1f;     // 기본 위치 흔들림 (cm)
        const float BaseAmpRot = 0.1f;     // 기본 회전 흔들림 (도)
        
        // 개수에 따른 증폭 (박스 1개당 10%~50% 씩 강해지도록 조절)
        // 많이 떨어질수록 강하게 흔들리지만, 너무 심하지 않게 Clamp 처리
        float IntensityMult = 1.0f + (float)DropCount * 0.1f; 
        
        // 전체 붕괴(StartIndex == 0)인 경우 임팩트를 위해 더 강하게 보정
        if (StartIndex == 0)
        {
            IntensityMult *= 2.0f; 
        }

        float FinalDuration = FMath::Clamp(BaseDuration + (DropCount * 0.05f), 0.2f, 1.0f);
        float FinalAmpLoc   = FMath::Clamp(BaseAmpLoc * IntensityMult, 0.1f, 5.0f);
        float FinalAmpRot   = FMath::Clamp(BaseAmpRot * IntensityMult, 0.5f, 5.0f);
        float Frequency     = 10.0f; 

        CameraManager->StartCameraShake(
            FinalDuration, 
            FinalAmpLoc, 
            FinalAmpRot, 
            Frequency, 
            10 // Priority (높음)
        );
    }
    // ==================

    FVector WorldLocation = GetWorldLocation();
    for (int32 i = StartIndex; i < ValidCargoCount; i++)
    {
        UStaticMeshComponent* CurrentCargo = CargoBoxes[i];
        if (CurrentCargo)
        {
            CurrentCargo->DetachFromParent(true);
            CurrentCargo->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            CurrentCargo->SetSimulatePhysics(true);

            FVector BoxWorldLocation = CurrentCargo->GetWorldLocation();
            FVector CollapseDirection = (BoxWorldLocation - WorldLocation);
            CollapseDirection += FVector(0, 0, 0.2f);
            CollapseDirection.Normalize();

            CurrentCargo->GetBodyInstance()->AddImpulse(CollapseDirection * CollapseImpulse, false);
            // CurrentCargo->GetBodyInstance()->SetLinearVelocity(CollapseDirection * CollapseImpulse);

            FVector RandomTorque = FVector(
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f),
                FMath::RandRange(-1.0f, 1.0f)
            );
            RandomTorque.Normalize();

            CurrentCargo->GetBodyInstance()->AddAngularImpulse(RandomTorque * RandomSpinImpulse, false);

            AVehicle* Vehicle = Cast<AVehicle>(GetOwner());
            if (Vehicle)
            {
                if (Vehicle->DropSoundComponent)
                {
                    float RandomPitch = 0.9f + FMath::RandRange(0.0f, 0.2f);
                    Vehicle->DropSoundComponent->Pitch = RandomPitch;
                    Vehicle->DropSoundComponent->Play(); 
                    Vehicle->DropSoundComponent->Play();
                }
            }
        }
    }

    ValidCargoCount = StartIndex;

    AVehicle* Vehicle = Cast<AVehicle>(GetOwner());
    UInputManager::GetInstance().SetGamepadVibration(0, 1.0f, 0.0f);
    Vehicle->GamepadVibrationTime = 0.3f;
    if (StartIndex == 0)
    {
        if (Vehicle)
        {
            FTransform VehicleTransform = Vehicle->GetRootComponent()->GetWorldTransform();
            FVector WorldUp = FVector(0, 0, 1);
            FVector VehicleUp = VehicleTransform.TransformVector(WorldUp);
            VehicleUp.Normalize();
            Vehicle->EjectDriver(VehicleUp * EjectionImpulse);
            Vehicle->GameOver(false);
        }
        CurrentState = ECargoState::Collapsed;
    }
}

void UCargoComponent::CollapseAll()
{
    CollapseFrom(0);
}