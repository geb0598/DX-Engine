#include "pch.h"
#include "Vehicle.h"

#include "AnimStateMachine.h"
#include "AnimStateMachineInstance.h"
#include "InputComponent.h"
#include "SkeletalMeshComponent.h"

AVehicle::AVehicle()
    : CurrentForwardInput(0.0f)
    , CurrentSteeringInput(0.0f)
    , CurrentTorqueInput(0.0f)
    , bIsBoosting(false)
    , bLeanLeftInput(false)
    , bLeanRightInput(false)
    , Driver(nullptr)
    , DriverStateMachine(nullptr)
    , StateId_Idle(-1)
    , bIsDriverEjected(false)
{
    ChassisMesh = CreateDefaultSubobject<UStaticMeshComponent>("ChassisMesh");
    FString ChassisFileName = GDataDir + "/Model/ShoppingCart/ShoppingCart.obj";
    ChassisMesh->SetStaticMesh(ChassisFileName);
    SetRootComponent(ChassisMesh);
    
    ChassisMesh->SetSimulatePhysics(true);
    ChassisMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

   /* FString WheelFileName[4] = {
        "/Model/Buggy/Buggy_Wheel_LF.obj",
        "/Model/Buggy/Buggy_Wheel_RF.obj",
        "/Model/Buggy/Buggy_Wheel_LB.obj",
        "/Model/Buggy/Buggy_Wheel_RB.obj"
    };*/
    FString WheelFileName[1] = {
        "/Model/ShoppingCart/Wheel/Wheel.obj",
    };

    //WheelMeshes.SetNum(4);
    WheelMeshes.SetNum(2);
    //for (int i = 0; i < 4; i++)
    for (int i = 0; i < 2; i++)
    {
        FName WheelName = "WheelMesh_" + std::to_string(i);
        WheelMeshes[i] = CreateDefaultSubobject<UStaticMeshComponent>(WheelName);
        WheelMeshes[i]->SetupAttachment(ChassisMesh);
        WheelMeshes[i]->SetSimulatePhysics(false);
        WheelMeshes[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WheelMeshes[i]->SetStaticMesh(GDataDir + WheelFileName[0]);
    }

    VehicleMovement = CreateDefaultSubobject<UVehicleMovementComponent>("VehicleMovement");

    SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    SpringArm->SetupAttachment(ChassisMesh);
    SpringArm->TargetArmLength = 20.0f;
    SpringArm->SocketOffset = FVector(-4.6f, 0.0f, 4.6f);
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(SpringArm);
    Camera->SetFarClipPlane(1000.f);

    Driver = CreateDefaultSubobject<USkeletalMeshComponent>("Driver");
    Driver->SetupAttachment(ChassisMesh);
    Driver->SetRelativeLocation(FVector(-1.43f, 0.0f, 0.27f));
    Driver->SetSkeletalMesh(GDataDir + "/Brian/Brian.fbx");
}

AVehicle::~AVehicle()
{
}

void AVehicle::BeginPlay()
{
    Super::BeginPlay();

    if (Driver)
    {
        Driver->UseStateMachine();
        DriverStateMachine = Driver->GetOrCreateStateMachine();

        if (DriverStateMachine)
        {
            auto& ResMgr = UResourceManager::GetInstance();
            auto* AnimIdle = ResMgr.Get<UAnimSequence>(GDataDir + "/Brian/Brian_Armature_Idle");
            auto* AnimLeft = ResMgr.Get<UAnimSequence>(GDataDir + "/Brian/Brian_Armature_Lean_Left");
            auto* AnimRight = ResMgr.Get<UAnimSequence>(GDataDir + "/Brian/Brian_Armature_Lean_Right");

            FAnimState StateIdle;
            StateIdle.Name = "Idle";
            StateIdle.bLooping = true;
            StateId_Idle = DriverStateMachine->AddState(StateIdle, AnimIdle);

            FAnimState StateLeft;
            StateLeft.Name = "LeanLeft";
            StateLeft.bLooping = false; // 1프레임 포즈 유지
            StateId_LeanLeft = DriverStateMachine->AddState(StateLeft, AnimLeft);

            FAnimState StateRight;
            StateRight.Name = "LeanRight";
            StateRight.bLooping = false; // 1프레임 포즈 유지
            StateId_LeanRight = DriverStateMachine->AddState(StateRight, AnimRight);

            DriverStateMachine->SetCurrentState(StateId_Idle, 0.0f);
        }
    }
}

void AVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (VehicleMovement)
    {
        VehicleMovement->SetSteeringInput(CurrentSteeringInput);

        VehicleMovement->SetUserTorque(CurrentTorqueInput);

        const float ForwardSpeed = VehicleMovement->GetForwardSpeed();
        const float SpeedThreshold = 2.0f; // @note 속도가 이 값보다 낮으면 멈춘 것으로 간주

        if (CurrentForwardInput > 0.0f)
        {
            VehicleMovement->SetGearToDrive();
            VehicleMovement->SetThrottleInput(CurrentForwardInput);
            VehicleMovement->SetBrakeInput(0.0f);
        }
        else if (CurrentForwardInput < 0.0f)
        {
            if (ForwardSpeed > SpeedThreshold) 
            {

                VehicleMovement->SetGearToDrive(); 
                VehicleMovement->SetThrottleInput(0.0f);
                VehicleMovement->SetBrakeInput(-CurrentForwardInput); 
            }
            else 
            {
                VehicleMovement->SetGearToReverse(); 
                VehicleMovement->SetThrottleInput(-CurrentForwardInput);
                VehicleMovement->SetBrakeInput(0.0f);
            }
        }
        else
        {
            VehicleMovement->SetGearToNeutralIfZeroVel();
            VehicleMovement->SetThrottleInput(0.0f);
            VehicleMovement->SetBrakeInput(0.0f);
        }
    }

    CurrentForwardInput = 0.0f;
    CurrentSteeringInput = 0.0f;
    CurrentTorqueInput = 0.0f;

    // 부스터 적용
    if (bIsBoosting && VehicleMovement)
    {
        VehicleMovement->ApplyBoostForce(BoostStrength);
    }

    SyncWheelVisuals();

    UpdateDriverAnimation(DeltaSeconds);
}

void AVehicle::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
    Super::SetupPlayerInputComponent(InInputComponent);

    if (!InInputComponent) return;

    // [축 바인딩] W/S로 가속/감속
    // W 키: Scale 1.0 -> 전진
    InInputComponent->BindAxis<AVehicle>("MoveForward_W", 'W', 1.0f, this, &AVehicle::MoveForward);
    // S 키: Scale -1.0 -> 후진/브레이크
    InInputComponent->BindAxis<AVehicle>("MoveForward_S", 'S', -1.0f, this, &AVehicle::MoveForward);

    // [축 바인딩] A/D로 조향
    // D 키: Scale 1.0 -> 우회전
    InInputComponent->BindAxis<AVehicle>("MoveRight_D", 'D', 1.0f, this, &AVehicle::MoveRight);
    // A 키: Scale -1.0 -> 좌회전
    InInputComponent->BindAxis<AVehicle>("MoveRight_A", 'A', -1.0f, this, &AVehicle::MoveRight);

    // [축 바인딩] Q/E로 Roll
    // Q 키: Scale 1.0f -> 좌로 기울어짐
    InInputComponent->BindAxis<AVehicle>("AddTorque_Q", 'Q', 1.0f, this, &AVehicle::AddTorque);
    // A 키: Scale -1.0f -> 우로 기울어짐
    InInputComponent->BindAxis<AVehicle>("AddTorque_E", 'E', -1.0f, this, &AVehicle::AddTorque);
    
    InInputComponent->BindAction<AVehicle>("LeanLeft",  'Q', this, &AVehicle::LeanLeftPressed,  &AVehicle::LeanLeftReleased);
    InInputComponent->BindAction<AVehicle>("LeanRight", 'E', this, &AVehicle::LeanRightPressed, &AVehicle::LeanRightReleased);
    
    // [액션 바인딩] Space로 핸드브레이크
    // VK_SPACE는 0x20
    InInputComponent->BindAction<AVehicle>("Handbrake", VK_SPACE, this, &AVehicle::HandbrakePressed, &AVehicle::HandbrakeReleased);

    // [액션 바인딩] Shift로 부스터
    // VK_SHIFT는 0x10
    InInputComponent->BindAction<AVehicle>("Boost", VK_SHIFT, this, &AVehicle::BoostPressed, &AVehicle::BoostReleased);
}

void AVehicle::EjectDriver(const FVector& Impulse)
{
    if (bIsDriverEjected || !Driver)
    {
        return;
    }

    // Driver->DetachFromParent(true);

    Driver->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    Driver->SetPhysicsMode(EPhysicsMode::Ragdoll);

    UPhysicsAsset* NewAsset = UResourceManager::GetInstance().Load<UPhysicsAsset>("Data/Physics/Brian.physicsasset");
    Driver->SetPhysicsAsset(NewAsset);

    if (!Impulse.IsZero())
    {
        Driver->AddImpulse(Impulse);
    }

    bIsDriverEjected = true;
}

void AVehicle::MoveForward(float Val)
{
    CurrentForwardInput += Val;
}

void AVehicle::MoveRight(float Val)
{
    CurrentSteeringInput += Val;
}

void AVehicle::AddTorque(float Val)
{
    CurrentTorqueInput += Val;
}

void AVehicle::LeanLeftPressed()  { bLeanLeftInput = true; }
void AVehicle::LeanLeftReleased() { bLeanLeftInput = false; }
void AVehicle::LeanRightPressed()  { bLeanRightInput = true; }
void AVehicle::LeanRightReleased() { bLeanRightInput = false; }

void AVehicle::UpdateDriverAnimation(float DeltaSeconds)
{
    if (bIsDriverEjected || !DriverStateMachine) return;

    // 현재 상태 인덱스
    int32 CurrentStateIdx = DriverStateMachine->GetCurrentStateIndex();
    
    // 목표 상태 및 블렌딩 시간 결정
    int32 DesiredStateIdx = StateId_Idle;
    float BlendTime = 0.2f;

    // 1. 입력에 따른 목표 상태 설정
    if (bLeanLeftInput)
    {
        DesiredStateIdx = StateId_LeanLeft;
        BlendTime = 0.15f; // [Fast In] 빠르게 기울임
    }
    else if (bLeanRightInput)
    {
        DesiredStateIdx = StateId_LeanRight;
        BlendTime = 0.15f; // [Fast In] 빠르게 기울임
    }
    else
    {
        // 입력 없음 -> IDLE 복귀
        DesiredStateIdx = StateId_Idle;

        // [Slow Out] 기울어진 상태였다면 천천히 복귀
        if (CurrentStateIdx == StateId_LeanLeft || CurrentStateIdx == StateId_LeanRight)
        {
            BlendTime = 0.5f; // 천천히 무게중심 회복
        }
    }

    if (CurrentStateIdx != DesiredStateIdx)
    {
        DriverStateMachine->SetCurrentState(DesiredStateIdx, BlendTime);
    }
}

void AVehicle::HandbrakePressed()
{
    if (VehicleMovement)
    {
        VehicleMovement->SetHandbrakeInput(true);
    }
}

void AVehicle::HandbrakeReleased()
{
    if (VehicleMovement)
    {
        VehicleMovement->SetHandbrakeInput(false);
    }
}

void AVehicle::BoostPressed()
{
    bIsBoosting = true;
}

void AVehicle::BoostReleased()
{
    bIsBoosting = false;
}

void AVehicle::SyncWheelVisuals()
{
    if (!VehicleMovement) return;

    //for (int i = 0; i < 4; i++)
    for (int i = 0; i < 2; i++)
    {
        if (WheelMeshes[i])
        {
            FTransform WheelTransform = VehicleMovement->GetWheelTransform(i);
            WheelMeshes[i]->SetRelativeLocation(WheelTransform.Translation);
            WheelMeshes[i]->SetRelativeRotation(WheelTransform.Rotation);
            WheelMeshes[i]->SetRelativeScale(WheelTransform.Scale3D);
        }
    }
}