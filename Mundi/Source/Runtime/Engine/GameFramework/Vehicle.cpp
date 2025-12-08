#include "pch.h"
#include "Vehicle.h"

#include "AnimStateMachine.h"
#include "AnimStateMachineInstance.h"
#include "AudioComponent.h"
#include "InputComponent.h"
#include "Landmine.h"
#include "GameVictoryVolume.h"
#include "PhysScene.h"
#include "SkeletalMeshComponent.h"
#include "LuaScriptComponent.h"
#include "ParticleSystemComponent.h"

class ALandmine;

AVehicle::AVehicle()
    : CurrentForwardInput(0.0f)
    , CurrentSteeringInput(0.0f)
    , CurrentTorqueInput(0.0f)
    , bIsBoosting(false)
    , bLeanLeftInput(false)
    , bLeanRightInput(false)
    , Driver(nullptr)
    , DriverStateMachine(nullptr)
    , SparkParticleComponent(nullptr)
    , StateId_Idle(-1)
    , bIsDriverEjected(false)
    , ScriptComponent(nullptr)
    , bSparkParticleActive(false)
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
   // SpringArm->SetupAttachment(ChassisMesh);
    SpringArm->TargetArmLength = 20.0f;
    SpringArm->SocketOffset = FVector(-4.6f, 0.0f, 4.6f);
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = true;
    SpringArm->bEnableCameraRotationLag = true;
    SpringArm->CameraLagSpeed = 10.0f;
    SpringArm->CameraRotationLagSpeed = 10.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(SpringArm);
    Camera->SetFarClipPlane(1000.f);

    Driver = CreateDefaultSubobject<USkeletalMeshComponent>("Driver");
    Driver->SetupAttachment(ChassisMesh);
    Driver->SetRelativeLocation(FVector(-1.43f, 0.0f, 0.27f));
    Driver->SetSkeletalMesh(GDataDir + "/Brian/Brian.fbx");

    ScriptComponent = CreateDefaultSubobject<ULuaScriptComponent>("GameModeLuaScript");
    ScriptComponent->ScriptFilePath = GDataDir + "/Scripts/Vehicle.lua";

    SparkParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("SparkParticle");
    SparkParticleComponent->SetupAttachment(WheelMeshes[1]);
    SparkParticleComponent->bAutoActivate = false;
    UParticleSystem* SparkParticle = UResourceManager::GetInstance().Load<UParticleSystem>(GDataDir + "/Particles/Spark.particle");
    SparkParticleComponent->SetTemplate(SparkParticle);

    // ===== Sound =====

    // 주행 사운드
    DriveSoundComponent = CreateDefaultSubobject<UAudioComponent>("DriveSound");
    USound* DriveSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/engine_heavy_loop.wav");
    DriveSoundComponent->SetSound(DriveSound);
    DriveSoundComponent->SetupAttachment(ChassisMesh);
    DriveSoundComponent->bIsLooping = true;
    DriveSoundComponent->bAutoPlay = false;

    // 충돌 사운드 (OneShot)
    HitSoundComponent = CreateDefaultSubobject<UAudioComponent>("HitSound");
    USound* HitSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/car-crash-sound.wav");
    HitSoundComponent->SetSound(HitSound);
    HitSoundComponent->SetupAttachment(ChassisMesh);
    HitSoundComponent->bIsLooping = false;
    HitSoundComponent->bAutoPlay = false;
    HitSoundComponent->Volume = 0.8f;

    // 운전자 사출 사운드 (OneShot)
    EjectSoundComponent = CreateDefaultSubobject<UAudioComponent>("EjectSound");
    USound* EjectSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/Die.wav");
    EjectSoundComponent->SetSound(EjectSound);
    EjectSoundComponent->SetupAttachment(Driver); 
    EjectSoundComponent->bIsLooping = false;
    EjectSoundComponent->bAutoPlay = false;

    // 화물 추락 사운드 
    DropSoundComponent = CreateDefaultSubobject<UAudioComponent>("DropSound");
    USound* DropSound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/box-crash-sound.wav");
    DropSoundComponent->SetSound(DropSound);
    DropSoundComponent->SetupAttachment(ChassisMesh);
    DropSoundComponent->bIsLooping = false;
    DropSoundComponent->bAutoPlay = false;
    DropSoundComponent->Volume = 0.8f;
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

        if (DriveSoundComponent)
        {
            DriveSoundComponent->Play();
        }

        if (ChassisMesh)
        {
            // @note BeginPlay() 중복 실행 버그로 인해서 일단 임시조치
            ChassisMesh->OnComponentHit.Clear();
            ChassisMesh->OnComponentHit.AddDynamic(this, &AVehicle::OnChassisHit);
        }
    }
}

void AVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);


    if (GamepadVibrationTime > 0.0f)
    {
        GamepadVibrationTime -= DeltaSeconds;
        if (GamepadVibrationTime <= 0.0f)
        {
            UInputManager::GetInstance().SetGamepadVibration(0, 0, 0);
        }
    }
    if (VehicleMovement)
    {
        VehicleMovement->SetSteeringInput(CurrentSteeringInput);

        VehicleMovement->SetUserTorque(CurrentTorqueInput);

        const float ForwardSpeed = VehicleMovement->GetForwardSpeed();
        // 자꾸 윌리해서 0.0으로 수정함
        const float SpeedThreshold = 0.0f; // @note 속도가 이 값보다 낮으면 멈춘 것으로 간주
        

        if (CurrentForwardInput > 0.0f)
        {
            if (ForwardSpeed < SpeedThreshold)
            {
                VehicleMovement->SetThrottleInput(0.0f);
                VehicleMovement->SetBrakeInput(CurrentForwardInput);
            }
            else
            {
                VehicleMovement->SetGearToDrive();
                VehicleMovement->SetThrottleInput(CurrentForwardInput);
                VehicleMovement->SetBrakeInput(0.0f);
            }
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

    // 파티클 활성화
    float SpeedMs = VehicleMovement->GetForwardSpeed();
    float SpeedKmh = std::abs(SpeedMs) * 3.6f;
    if (SpeedKmh >= SparkParticleSpawnSpeed)
    {
        if (!bSparkParticleActive)
        {
            SparkParticleComponent->ActivateSystem();
            UInputManager::GetInstance().SetGamepadVibration(0, 0.0f, 0.5f);
            bSparkParticleActive = true;
        }
    }
    else
    {
        if (bSparkParticleActive)
        {
            SparkParticleComponent->FinishSystem();
            UInputManager::GetInstance().SetGamepadVibration(0, 0.0f, 0.0f);
            bSparkParticleActive = false;
        }
    }

    if (DriveSoundComponent && VehicleMovement)
    {
        float CurrentRPM = VehicleMovement->GetEngineRPM();
        float MaxRPM = 6000.0f;
        
        // 2. RPM을 0.0 ~ 1.0 사이로 정규화 (Normalization)
        // 엔진은 시동이 켜지면 최소 RPM(아이들링, 약 800~1000)이 있으므로 0부터 시작하지 않음
        float NormalizedRPM = FMath::Clamp(CurrentRPM / MaxRPM, 0.0f, 1.0f);

        // 3. 피치 계산
        // 아이들링(낮은 RPM)일 때: 0.6 ~ 0.8
        // 레드존(최대 RPM)일 때: 1.5 ~ 2.0
        float BasePitch = 0.6f;
        float PitchRange = 1.4f; // 0.6 + 1.4 = 2.0
        
        float TargetPitch = BasePitch + (NormalizedRPM * PitchRange);

        // 4. 볼륨 계산 (볼륨은 RPM보다는 '부하(Load)'나 'Throttle'에 반응하는 게 좋지만, 일단 RPM+속도 섞어서)
        // RPM이 높으면 시끄럽게, 하지만 너무 작아지지 않게 기본값 확보
        float TargetVolume = 0.2f + (NormalizedRPM * 0.6f);

        // 5. 적용
        DriveSoundComponent->Pitch = TargetPitch;
        DriveSoundComponent->Volume = FMath::Clamp(TargetVolume, 0.0f, 1.0f);
    }

    SyncWheelVisuals();

    UpdateDriverAnimation(DeltaSeconds);

    CheckWheelInteractions();
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

    // LTrigger 감속 RTrigger 가속 (트리거 0~1사이로 정규화되므로 Scale곱해줌)
    InInputComponent->BindAxis<AVehicle>("MoveForward_RTRIGGER", (int32)EGamepadAxis::RTRIGGER, 1.0f, this, &AVehicle::MoveForward);
    InInputComponent->BindAxis<AVehicle>("MoveForward_LTRIGGER", (int32)EGamepadAxis::LTRIGGER, -1.0f, this, &AVehicle::MoveForward);

    // [축 바인딩] A/D로 조향
    // D 키: Scale 1.0 -> 우회전
    InInputComponent->BindAxis<AVehicle>("MoveRight_D", 'D', 1.0f, this, &AVehicle::MoveRight);
    // A 키: Scale -1.0 -> 좌회전
    InInputComponent->BindAxis<AVehicle>("MoveRight_A", 'A', -1.0f, this, &AVehicle::MoveRight);

    InInputComponent->BindAxis<AVehicle>("MoveRight_LSTICK_X", (int32)EGamepadAxis::LSTICK_X, 1.0f, this, &AVehicle::MoveRight);


    // [축 바인딩] Q/E로 Roll
    // Q 키: Scale 1.0f -> 좌로 기울어짐
    InInputComponent->BindAxis<AVehicle>("AddTorque_Q", 'Q', 1.0f, this, &AVehicle::AddTorque);
    // A 키: Scale -1.0f -> 우로 기울어짐
    InInputComponent->BindAxis<AVehicle>("AddTorque_E", 'E', -1.0f, this, &AVehicle::AddTorque);

    InInputComponent->BindAxis<AVehicle>("AddTorque_Y", (int32)EGamepadButton::Y, 1.0f, this, &AVehicle::AddTorque);
    InInputComponent->BindAxis<AVehicle>("AddTorque_A", (int32)EGamepadButton::A, -1.0f, this, &AVehicle::AddTorque);
    
    InInputComponent->BindAction<AVehicle>("LeanLeft",  'Q', this, &AVehicle::LeanLeftPressed,  &AVehicle::LeanLeftReleased);
    InInputComponent->BindAction<AVehicle>("LeanRight", 'E', this, &AVehicle::LeanRightPressed, &AVehicle::LeanRightReleased);

    InInputComponent->BindAction<AVehicle>("LeanLeft_Y", (int32)EGamepadButton::Y, this, &AVehicle::LeanLeftPressed, &AVehicle::LeanLeftReleased);
    InInputComponent->BindAction<AVehicle>("LeanRight_A", (int32)EGamepadButton::A, this, &AVehicle::LeanRightPressed, &AVehicle::LeanRightReleased);
    
    InInputComponent->BindAction<AVehicle>("Jump", 'F', this, &AVehicle::JumpPressed, &AVehicle::JumpReleased);
    InInputComponent->BindAction<AVehicle>("Jump", (int32)EGamepadButton::X, this, &AVehicle::JumpPressed, &AVehicle::JumpReleased);


    // [액션 바인딩] Space로 핸드브레이크
    // VK_SPACE는 0x20
    InInputComponent->BindAction<AVehicle>("Handbrake", VK_SPACE, this, &AVehicle::HandbrakePressed, &AVehicle::HandbrakeReleased);
    InInputComponent->BindAction<AVehicle>("Handbrake", (int32)EGamepadButton::B, this, &AVehicle::HandbrakePressed, &AVehicle::HandbrakeReleased);

    // [액션 바인딩] Shift로 부스터
    // VK_SHIFT는 0x10
    InInputComponent->BindAction<AVehicle>("Boost", VK_SHIFT, this, &AVehicle::BoostPressed, &AVehicle::BoostReleased);
    InInputComponent->BindAction<AVehicle>("Boost", (int32)EGamepadButton::R_SHOULDER, this, &AVehicle::BoostPressed, &AVehicle::BoostReleased);
}

void AVehicle::EjectDriver(const FVector& Impulse)
{
    if (bIsDriverEjected || !Driver)
    {
        return;
    }

    // ===== 카메라 연출 =====
    UWorld* World = GetWorld();
    if (World)
    {
        // 0.2배속으로 3초간 진행 (너무 느리면 답답하므로 0.2~0.3 추천)
        World->RequestSlomo(3.0f, 0.2f);
    }

    // =====================
    Driver->DetachFromParent(true);

    Driver->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    UPhysicsAsset* NewAsset = UResourceManager::GetInstance().Load<UPhysicsAsset>("Data/Physics/Brian.physicsasset");
    Driver->SetPhysicsAsset(NewAsset);
    
    Driver->SetPhysicsMode(EPhysicsMode::Ragdoll);

    if (!Impulse.IsZero())
    {
        Driver->AddImpulse(Impulse);
    }

    if (EjectSoundComponent)
    {
        EjectSoundComponent->Play();
    }

    bIsDriverEjected = true;
}

UCameraComponent* AVehicle::GetCamera()
{
    return Camera;
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

void AVehicle::JumpPressed()
{
    if (VehicleMovement)
    {
        VehicleMovement->SetJumpInput(true);
    }
}

void AVehicle::JumpReleased()
{
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

void AVehicle::CheckWheelInteractions()
{
    if (!VehicleMovement || !ChassisMesh) return;
    
    UWorld* World = GetWorld();
    if (!World) return;

    FPhysScene* PhysScene = World->GetPhysicsScene();
    if (!PhysScene) return;

    UVehicleWheel* Wheels[2] = { VehicleMovement->VehicleWheel0, VehicleMovement->VehicleWheel1 };

    for (int i = 0; i < 2; ++i)
    {
        FTransform WheelTrans = VehicleMovement->GetWheelTransform(i);
        FVector WheelPos = ChassisMesh->GetWorldTransform().TransformPosition(WheelTrans.Translation);
        FVector SweepStart = WheelPos + FVector(0, 0, 0.1f);
        FVector SweepEnd = WheelPos - FVector(0, 0, 0.2f);

        float WheelRadius = (Wheels[i]) ? Wheels[i]->WheelRadius : 0.3f;

        FHitResult Hit;
        bool bHit = PhysScene->SweepSingleSphere(
            SweepStart,
            SweepEnd,
            WheelRadius, // 반지름
            Hit,      // 결과
            this         // IgnoreActor (내 차체는 무시)
        );

        if (bHit && Hit.Actor.Get()) 
        {
            if (ALandmine* Mine = Cast<ALandmine>(Hit.Actor.Get()))
            {
                UPrimitiveComponent* MineComp = Mine->GetMeshComponent();
                if (MineComp)
                {
                    FHitResult FakeHit = Hit;
                    FakeHit.Actor = this;            
                    FakeHit.Component = ChassisMesh; 

                    MineComp->DispatchBlockingHit(this, ChassisMesh, FVector::Zero(), FakeHit);
                }
            }
            else if (AGameVictoryVolume* VictoryVolume = Cast<AGameVictoryVolume>(Hit.Actor.Get()))
            {
                // 우선 지정된 TriggerVolume으로 디스패치, 없으면 Hit된 컴포넌트로 fallback 시도.
                UPrimitiveComponent* VictoryComp = VictoryVolume->TriggerVolume;
                if (!VictoryComp)
                {
                    VictoryComp = Cast<UPrimitiveComponent>(Hit.Component.Get());
                }

                if (VictoryComp)
                {
                    FHitResult FakeHit = Hit;
                    FakeHit.Actor = this;
                    FakeHit.Component = ChassisMesh;

                    VictoryComp->DispatchBlockingHit(this, ChassisMesh, FVector::Zero(), FakeHit);
                }
            }
        }
    }
}

void AVehicle::OnChassisHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor == this || bIsDriverEjected) return;

    float ImpactForce = NormalImpulse.Size();

    if (HitSoundComponent)
    {
        float RandomPitch = 0.9f + FMath::RandRange(0.0f, 0.2f);
        HitSoundComponent->Pitch = RandomPitch;
        HitSoundComponent->Play(); 
    }
    UInputManager::GetInstance().SetGamepadVibration(0, 1.0f, 0.0f);
    GamepadVibrationTime = 0.3f;
    
}

void AVehicle::SyncWheelVisuals()
{
    if (!VehicleMovement) return;

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
