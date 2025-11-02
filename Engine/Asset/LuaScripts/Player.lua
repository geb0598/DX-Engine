------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local moveSpeed = 5.0
local rotationSpeed = 80.0 -- 마우스 민감도 (초당 80도)

-- FVector(Roll_X, Pitch_Y, Yaw_Z) 형식으로 오일러 각을 저장
local currentRotation = FVector(0, 0, 0)

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    Log("Player.lua BeginPlay. Owner:", Owner.UUID)
    
    -- Owner.Rotation (FQuaternion) -> ToEuler() -> FVector(Roll, Pitch, Yaw)
    currentRotation = Owner.Rotation:ToEuler()
end


---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    --- 이동 ---
    local ForwardScale = 0.0
    local RightScale = 0.0
    local Forward = Owner:GetActorForwardVector()
    local Right = Owner:GetActorRightVector()
    
    if IsKeyDown(Keys.W) then
        ForwardScale = ForwardScale + 1.0
    end
    if IsKeyDown(Keys.S) then
        ForwardScale = ForwardScale - 1.0
    end
    if IsKeyDown(Keys.A) then
        RightScale = RightScale - 1.0
    end
    if IsKeyDown(Keys.D) then
        RightScale = RightScale + 1.0
    end

    MoveDirection = Forward * ForwardScale + Right * RightScale
    MoveDirection.Z = 0
    if MoveDirection:Length() > 0.0 then
        MoveDirection:Normalize()
        Owner.Location = Owner.Location + (MoveDirection * moveSpeed * dt)
    end
    
    --- 회전 ---
    local mouseDelta = GetMouseDelta()
    
    currentRotation.X = 0
    currentRotation.Y = currentRotation.Y - (mouseDelta.Y * rotationSpeed * dt)
    currentRotation.Z = currentRotation.Z + (mouseDelta.X * rotationSpeed * dt)
    -- (Pitch는 짐벌 락 방지를 위해 -89 ~ 89도로 Clamp하는 것이 좋습니다)
    
    Owner.Rotation = FQuaternion.FromEuler(currentRotation)
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
end

------------------------------------------------------------------
-- [Delegate 이벤트 콜백]
------------------------------------------------------------------

function OnActorBeginOverlap(overlappedActor, otherActor)
    -- Log("Overlap started with: " .. otherActor.Name)
end

function OnActorEndOverlap(overlappedActor, otherActor)
    -- Log("Overlap ended with: " .. otherActor.Name)
end

function OnActorHit(hitActor, otherActor, normalImpulse, hit)
    -- Log("Hit by: " .. otherActor.Name)
end