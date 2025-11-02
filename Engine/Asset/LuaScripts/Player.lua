------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

Log("Player.lua: Script loaded and initialized")

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    Log("Player.lua BeginPlay. Owner:", Owner.UUID)
end

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    -- 여기서 게임 로직 처리
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
