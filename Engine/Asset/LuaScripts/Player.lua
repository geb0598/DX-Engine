------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local moveSpeed = 5.0
local rotationSpeed = 80.0 -- 마우스 민감도 (초당 80도)

-- FVector(Roll_X, Pitch_Y, Yaw_Z) 형식으로 오일러 각을 저장
local currentRotation = FVector(0, 0, 0)

-- Light Intensity 흡수 관련 변수
local accumulatedLightIntensity = 0.0
local lightIntensityThreshold = 30.0  -- Enemy 스폰 임계값
local currentLuminance = 0.0          -- 현재 조명 세기
local logFrameCounter = 0             -- 로그 출력용 프레임 카운터

Log("Player.lua: Script loaded and initialized")

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    Log("Player.lua BeginPlay. Owner:", Owner.UUID)

    -- Owner.Rotation (FQuaternion) -> ToEuler() -> FVector(Roll, Pitch, Yaw)
    currentRotation = Owner.Rotation:ToEuler()
    Log("Player: Rotation initialized")

    -- LightSensorComponent 찾아서 Delegate 바인딩
    Log("Player: Attempting to get owned components...")
    local success, components = pcall(function() return Owner:GetOwnedComponents() end)
    if not success then
        Log("Player: ERROR - GetOwnedComponents() failed: " .. tostring(components))
        return
    end

    Log("Player: Found " .. #components .. " components")

    for i = 1, #components do
        local comp = components[i]
        Log("Player: Checking component " .. i .. ": " .. tostring(comp))

        -- LightSensorComponent 타입 체크 (OnLightIntensityChanged 멤버 존재 확인)
        if comp.OnLightIntensityChanged then
            -- Delegate에 Lua 콜백 함수 등록
            comp.OnLightIntensityChanged = OnLightIntensityChanged
            Log("Player: LightSensorComponent Delegate bound!")
            break
        end
    end
end


-- Tick 디버깅용 프레임 카운터
local tickFrameCounter = 0

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    -- Tick 호출 확인용 로그 (120프레임마다)
    tickFrameCounter = tickFrameCounter + 1
    if tickFrameCounter >= 120 then
        Log(string.format("Player: Tick called (frame %d), dt=%.4f", tickFrameCounter, dt))
        tickFrameCounter = 0
    end

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

    --- Light Intensity 흡수 ---
    AbsorbLightIntensity(dt)
end

---
-- Luminance 기반 Light Intensity 흡수 함수
-- @param dt (float): Delta Time
---
function AbsorbLightIntensity(dt)
    -- Luminance 누적 (dt를 곱해 시간에 비례)
    accumulatedLightIntensity = accumulatedLightIntensity + (currentLuminance * dt)

    -- 120프레임마다 현재 상태 출력 (60fps 기준 약 2초마다)
    logFrameCounter = logFrameCounter + 1
    if logFrameCounter >= 120 then
        Log(string.format("Player: CurrentLuminance=%.3f, Accumulated=%.2f/%.2f",
            currentLuminance, accumulatedLightIntensity, lightIntensityThreshold))
        logFrameCounter = 0
    end

    -- 임계값 도달 시 Enemy 스폰
    if accumulatedLightIntensity >= lightIntensityThreshold then
        Log(string.format("Player: Threshold reached! (%.2f >= %.2f) - Spawning Enemy",
            accumulatedLightIntensity, lightIntensityThreshold))
        SpawnEnemy()
        accumulatedLightIntensity = 0.0  -- 누적값 리셋
    end
end

---
-- LightSensorComponent의 조명 세기 변경 Delegate 콜백
-- @param current (float): 현재 측정된 Luminance 값
-- @param previous (float): 이전 측정된 Luminance 값
---
function OnLightIntensityChanged(current, previous)
    currentLuminance = current
end

---
-- Enemy를 스폰하는 함수 (현재는 로그만 출력)
---
function SpawnEnemy()
    -- Player 앞쪽 5 유닛 거리에 스폰할 위치 계산
    local spawnOffset = Owner:GetActorForwardVector() * 5.0
    local spawnLocation = Owner.Location + spawnOffset

    Log(string.format("Player: SpawnEnemy() called at position (%.1f, %.1f, %.1f)",
        spawnLocation.X, spawnLocation.Y, spawnLocation.Z))

    -- TODO: 실제 Enemy 스폰 구현
    -- local world = GetWorld()
    -- local enemyTemplate = world:FindTemplateActorOfName("Enemy")
    -- if enemyTemplate then
    --     local enemy = enemyTemplate:DuplicateFromTemplate(world:GetLevel(), spawnLocation)
    -- end
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