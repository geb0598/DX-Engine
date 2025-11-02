------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

-- [Movement]
local moveSpeed = 10.0
local rotationSpeed = 80.0
local currentRotation = FVector(0, 0, 0)

-- [Health]
local MaxHP = 5
local currentHP = 0
local gameMode = nils

---
-- [Health] HP가 0 이하가 되었는지 확인하고 GameMode의 EndGame을 호출
---
local function CheckForDeath()
    if currentHP <= 0 then
        if gameMode and gameMode.IsGameRunning then
            Log("PlayerHealth: HP is 0. Calling EndGame.")
            gameMode:EndGame()
        end
    end
end

---
-- [Health] 플레이어에게 데미지를 입힘
---
local function TakeDamage(damage)
    if currentHP <= 0 then return end

    currentHP = currentHP - damage
    Log(string.format("PlayerHealth: Took %d damage. HP remaining: %d", damage, currentHP))

    CheckForDeath()
end

local function EndedTest()
    Log("GameEnded Delegate!")
end

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    -- [Health] HP 및 GameMode 초기화
    currentHP = MaxHP
    local world = GetWorld()
    if world then
        gameMode = world:GetGameMode()
        gameMode:StartGame()
        gameMode.OnGameEnded = EndedTest
    end

    -- [Movement] 회전값 초기화
    currentRotation = Owner.Rotation:ToEuler()

    Log(string.format("Player.lua BeginPlay. Owner: %s, HP: %d", Owner.UUID, currentHP))
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
    Log("Player.lua EndPlay.")
end

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    Log("Player: Overlap started with: " .. otherActor.Name)

    -- [Health] 적 태그 확인
    if otherActor.Tag == CollisionTag.Enemy then
        TakeDamage(1)
    end

    -- [Collision] 벽 태그 확인
    if otherActor.Tag == CollisionTag.Wall then
        Log("Hit a wall! Reverting location.")
    end
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
---
function OnActorEndOverlap(overlappedActor, otherActor)
    -- Log("Overlap ended with: " .. otherActor.Name)
end
