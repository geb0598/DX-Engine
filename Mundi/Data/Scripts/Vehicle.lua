local Vehicle = nil
local MovementComponent = nil
local SpringArm = nil
local CargoComponent = nil
local SkeletalMeshComponent = nil

-- TODO: 화물 기본으로 붙이는 시점에 동적으로 계산하도록 수정
local InitialCargoCount = 20
local InitialSpringArmLength = 15
local InitialSocketOffsetZ = 4.6
function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID)
    Vehicle = GetOwnerAs(Obj, "AVehicle")
    MovementComponent = GetComponent(Obj, "UVehicleMovementComponent")
    SpringArm = GetComponent(Obj, "USpringArmComponent")
    SkeletalMeshComponent = GetComponent(Obj, "USkeletalMeshComponent")
end

function EndPlay()
    print("[EndPlay] " .. Obj.UUID)
end

function OnBeginOverlap(OtherActor)
    --[[Obj:PrintLocation()]]--
end

function OnEndOverlap(OtherActor)
    --[[Obj:PrintLocation()]]--
end

function Tick(dt)
    if InputManager:IsKeyPressed('R')
    then
        --디버깅용 회전 초기화
        Obj.Rotation = Vector(0,0,0)
    end
    
    -- TODO: 화물 기본으로 붙이면 BeginPlay로 옮기기
    CargoComponent = GetComponent(Obj, "UCargoComponent")
    if CargoComponent 
    then
        local NumCargo = CargoComponent:GetValidCargoCount()
        if(NumCargo == 0)
        then
            SpringArm:DetachFromParent(true)
            SpringArm:SetOwner(nil)
            SpringArm:SetWorldLocation(SkeletalMeshComponent:GetBoneWorldLocation(0))
            SpringArm:SetRelativeRotationEuler(Vector(0,30.0,0))
            SpringArm:AddLocalLocation(Vector(-3, 0, 0))
            
        else
            local RealTargetArmLength = InitialSpringArmLength * (NumCargo / InitialCargoCount * 0.7 + 0.3)
            local TargetSocketOffsetZ  = InitialSocketOffsetZ * (NumCargo / InitialCargoCount * 0.6 + 0.4)
            SpringArm.TargetArmLength = SpringArm.TargetArmLength + dt * (RealTargetArmLength - SpringArm.TargetArmLength)
            local SocketOffsetZ = SpringArm.SocketOffset.Z + dt * (TargetSocketOffsetZ - SpringArm.SocketOffset.Z)
            SpringArm.SocketOffset = Vector(SpringArm.SocketOffset.X, SpringArm.SocketOffset.Y, SocketOffsetZ)
        end
        
    end
    --[[Obj:PrintLocation()]]--
    --[[print("[Tick] ")]]--
end