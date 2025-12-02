#include "pch.h"
#include "VehicleMovementComponent.h"
#include "SceneComponent.h"
#include "PrimitiveComponent.h"
#include "BodyInstance.h"
#include "PhysScene.h"
#include "Vehicle/VehicleWheel.h"

using namespace physx;

UVehicleMovementComponent::UVehicleMovementComponent()
    : PVehicleDrive(nullptr)
    , PInputData(nullptr)
    , PWheelsSimData(nullptr)
    , PDriveSimData(nullptr)
{
    WheelSetups.SetNum(4); 
}

UVehicleMovementComponent::~UVehicleMovementComponent()
{
}

void UVehicleMovementComponent::InitializeComponent()
{
    Super::InitializeComponent();

    PInputData = new PxVehicleDrive4WRawInputData();

    SetupVehicle();
}

void UVehicleMovementComponent::SetupVehicle()
{
    UPrimitiveComponent* MeshComp = Cast<UPrimitiveComponent>(UpdatedComponent);
    if (!MeshComp || !MeshComp->GetBodyInstance())
    {
        return;
    }

    FBodyInstance* BodyInst = MeshComp->GetBodyInstance();
    PxRigidDynamic* RigidActor = BodyInst->RigidActor ? BodyInst->RigidActor->is<PxRigidDynamic>() : nullptr;

    if (!RigidActor)
    {
        return;
    }

    PWheelsSimData = physx::PxVehicleWheelsSimData::allocate(4);
    for (int32 i = 0; i < 4; i++)
    {
        if (WheelSetups[i])
        {
            PWheelsSimData->setWheelData(i, WheelSetups[i]->GetPxWheelData());
            PWheelsSimData->setSuspensionData(i, WheelSetups[i]->GetPxSuspensionData());
            
            // 휠 오프셋 설정 (매우 중요)
            // 예시: 메쉬 소켓 위치 등을 가져와서 설정해야 함. 임시로 하드코딩
            // FL, FR, RL, RR 순서
            float X = (i < 2) ? 1.5f : -1.5f;      // 앞/뒤
            float Y = (i % 2 == 0) ? -1.0f : 1.0f; // 좌/우
            PWheelsSimData->setWheelCentreOffset(i, PxVec3(X, Y, -0.5f)); 
            
            PWheelsSimData->setSuspTravelDirection(i, PxVec3(0, 0, -1));

            // @note PxRigidActor의 Shape 배열에서 0, 1, 2, 3번 인덱스가 바퀴 Shape이어야 함.
            //       Actor 생성 시 바퀴 Shape 4개를 먼저 추가하고, 차체를 추가해야 함.
            PWheelsSimData->setWheelShapeMapping(i, i);
        }
    }

    physx::PxVehicleDriveSimData4W DriveSimData;
    DriveSimData.setEngineData(EngineSetup.GetPxEngineData());
    // @note 디폴트 객체 생성 후 전달
    DriveSimData.setGearsData(PxVehicleGearsData());
    
    PxVehicleDifferential4WData DiffData;
    DiffData.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
    DriveSimData.setDiffData(DiffData);

    PVehicleDrive = physx::PxVehicleDrive4W::allocate(4);
    
    PVehicleDrive->setup(GPhysXSDK, RigidActor, *PWheelsSimData, DriveSimData, 0);
}

void UVehicleMovementComponent::TickComponent(float DeltaSeconds)
{
    Super::TickComponent(DeltaSeconds);

    if (!PVehicleDrive || !UpdatedComponent) { return; }

    PerformSuspensionRaycasts(DeltaSeconds);

    PxScene* Scene = PVehicleDrive->getRigidDynamicActor()->getScene();
    PxVec3 Gravity = Scene ? Scene->getGravity() : PxVec3(0, 0, -9.81f);

    static PxVehicleDrivableSurfaceToTireFrictionPairs* FrictionPairs = nullptr;
    if (!FrictionPairs)
    {
        // ... FrictionPairs 초기화 로직 (복잡하므로 생략, null이면 기본 마찰) ...
    }

    PxVehicleUpdates(DeltaSeconds, Gravity, *FrictionPairs, 1, (PxVehicleWheels**)&PVehicleDrive, nullptr);
}

void UVehicleMovementComponent::OnRegister(UWorld* InWorld)
{
    Super::OnRegister(InWorld);
}

void UVehicleMovementComponent::OnUnregister()
{
    Super::OnUnregister();
    if (PInputData) { delete PInputData; PInputData = nullptr; }
    if (PWheelsSimData) { PWheelsSimData->free(); PWheelsSimData = nullptr; }
    // PxVehicleDrive4W는 free()를 호출해야 함 (PxBase 상속)
    if (PVehicleDrive) { PVehicleDrive->free(); PVehicleDrive = nullptr; }
}

void UVehicleMovementComponent::PerformSuspensionRaycasts(float DeltaTime)
{
    // PhysX Scene Query를 사용하여 바퀴 아래를 검사
    PxVehicleWheels* Vehicles[1] = { PVehicleDrive };
    
    // 레이캐스트 결과를 담을 버퍼
    PxRaycastQueryResult RaycastResults[4];
    PxVehicleSuspensionRaycasts(
        nullptr, // BatchQuery (널이면 내부적으로 즉시 쿼리 수행 - 성능상 BatchQuery 권장)
        1, 
        Vehicles, 
        4, // 쿼리 개수 (바퀴 수)
        RaycastResults
    );
}

void UVehicleMovementComponent::SetThrottleInput(float Throttle)
{
    if (PInputData) PInputData->setAnalogAccel(Throttle);
}

void UVehicleMovementComponent::SetSteeringInput(float Steering)
{
    if (PInputData) PInputData->setAnalogSteer(Steering);
}

void UVehicleMovementComponent::SetBrakeInput(float Brake)
{
    if (PInputData) PInputData->setAnalogBrake(Brake);
}

void UVehicleMovementComponent::SetHandbrakeInput(bool bNewHandbrake)
{
    if (PInputData) PInputData->setAnalogHandbrake(bNewHandbrake ? 1.0f : 0.0f);
}

FTransform UVehicleMovementComponent::GetWheelTransform(int32 WheelIndex) const
{
    if (!PVehicleDrive || WheelIndex >= 4)
    {
        return FTransform();
    }

    PxI32 ShapeIndex = PVehicleDrive->mWheelsSimData.getWheelShapeMapping(WheelIndex);
    
    if (ShapeIndex < 0)
    {
        return FTransform();
    }

    PxRigidDynamic* Actor = PVehicleDrive->getRigidDynamicActor();
    if (!Actor) 
    {
        return FTransform();
    }

    PxShape* WheelShape = nullptr;
    if (Actor->getShapes(&WheelShape, 1, ShapeIndex) == 1 && WheelShape)
    {
        return P2UTransform(WheelShape->getLocalPose());
    }

    return FTransform();
}