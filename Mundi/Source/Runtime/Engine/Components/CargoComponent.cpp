#include "pch.h"
#include "CargoComponent.h"

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

    FTransform CompTransform = GetWorldTransform();
    FVector WorldUp = FVector(0, 0, 1);
    FVector CompUp = CompTransform.TransformVector(WorldUp);
    CompUp.Normalize();

    float LeanAngle = RadiansToDegrees(std::acos(FVector::Dot(CompUp, WorldUp)));

    if (LeanAngle > CriticalAngle)
    {
        CollapseAll();
        return;
    }

    float PitchInput = CompUp.X;
    float RollInput = CompUp.Y;

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
        
        // "위험해!" UI 연출을 넣기 좋은 타이밍 (화면 붉어짐, 짐 흔들림 소리 등)
        UE_LOG("위험해!!!");
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
        }
    }

    ValidCargoCount = StartIndex;

    if (StartIndex == 0)
    {
        CurrentState = ECargoState::Collapsed;
    }
}

void UCargoComponent::CollapseAll()
{
    AVehicle* Vehicle = Cast<AVehicle>(GetOwner());
    if (Vehicle)
    {
        FTransform VehicleTransform = Vehicle->GetRootComponent()->GetWorldTransform();
        FVector WorldUp = FVector(0, 0, 1);
        FVector VehicleUp = VehicleTransform.TransformVector(WorldUp);
        VehicleUp.Normalize();
        Vehicle->EjectDriver(VehicleUp * EjectionImpulse);
    }
    CollapseFrom(0);
}