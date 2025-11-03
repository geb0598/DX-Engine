------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local bStarted = false

-- [Movement]
local moveSpeed = 500.0
local rotationSpeed = 30.0
local currentRotation = FVector(0, 0, 0)

-- [Health]
local MaxHP = 5
local currentHP = 0
local gameMode = nil

-- [Light Exposure]
local LightCriticalPoint = 1.0
local MaxLightExposureTime = 5.0
local CurrentLightExposureTime = 5.0
local bLightWarningShown = false
local CurrentLightLevel = 0.0  -- 현재 빛 강도 저장

-- [Animation]
local totalTime = 0.0   -- 전체 경과 시간 (애니메이션용)

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

function OnLightIntensityChanged(current, previous)
    -- 현재 빛 강도만 저장
    CurrentLightLevel = current
    
    -- 로그 출력
    local delta = current - previous
    if delta > 0 then
        Log(string.format("Light Changed: %.3f -> %.3f (Delta: %.3f)", previous, current, delta))
    end
end

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    bStarted = false
    local world = GetWorld()
    if world then
        gameMode = world:GetGameMode()
        gameMode.OnGameStarted = StartGame
        gameMode.OnGameEnded = EndedTest
        gameMode:StartGame()
    end
    StartGame()
end

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
	if bStarted == false then
		return
	end

	totalTime = totalTime + dt
	Movement(dt)
	UpdateLightExposure(dt)
	DrawUI()
end

function Movement(dt)
    --- [Movement] 이동 ---
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
    MoveDirection.Z = 0 -- (Z축 이동 방지)

    if MoveDirection:Length() > 0.0 then
        MoveDirection:Normalize()
        local targetLocation = Owner.Location + (MoveDirection * moveSpeed * dt)

        local world = GetWorld()
        local level = nil
        if world then
            level = world:GetLevel()
        end

        local hitResult = nil
        if level then
            hitResult = level:SweepActorSingle(Owner, targetLocation, CollisionTag.Wall)
        end

        if hitResult == nil then
            Owner.Location = targetLocation
        else
            Log("Movement blocked by wall.")
        end
    end
    
    --- [Movement] 회전 ---
    local mouseDelta = GetMouseDelta()
    
    currentRotation.X = 0
    local newPitch = currentRotation.Y - (mouseDelta.Y * rotationSpeed * dt)
    currentRotation.Y = Clamp(newPitch, -89.0, 89.0)
    currentRotation.Z = currentRotation.Z + (mouseDelta.X * rotationSpeed * dt)
    
    Owner.Rotation = FQuaternion.FromEuler(currentRotation)
end

---
-- [Light Exposure] 매 프레임 빛 노출 시간 업데이트
---
function UpdateLightExposure(dt)
    -- LightCriticalPoint 기준으로 노출 시간 조정
    if CurrentLightLevel >= LightCriticalPoint then
        -- 밝은 곳: 노출 시간 감소
        CurrentLightExposureTime = CurrentLightExposureTime - dt

        if CurrentLightExposureTime < 0 then
            CurrentLightExposureTime = 0
        end

        -- 0초가 되면 적 스폰 요청 + 경고 로그
        if CurrentLightExposureTime <= 0 then
            RequestSpawnEnemy()

            if not bLightWarningShown then
                Log("WARNING: Light Exposure Time has reached 0! Spawning enemies!")
                bLightWarningShown = true
            end
        end
    else
        -- 어두운 곳: 노출 시간 회복
        CurrentLightExposureTime = CurrentLightExposureTime + dt

        if CurrentLightExposureTime > MaxLightExposureTime then
            CurrentLightExposureTime = MaxLightExposureTime
        end

        -- 회복되면 경고 플래그 리셋
        if CurrentLightExposureTime > 0 then
            bLightWarningShown = false
        end
    end
end

---
-- [UI] HP 바 및 Light Exposure 바를 그립니다.
---
function DrawUI()
    local ui_x = 80.0
    local ui_y = 60.0
    local bar_width = 200.0
    local bar_height = 20.0
    local bar_spacing = 10.0
    
    -- ============ HP BAR ============
    local hp_ratio = currentHP / MaxHP
    
    -- HP 색상 결정
    local hp_c_r, hp_c_g, hp_c_b = 0.1, 1.0, 0.1
    if hp_ratio <= 0.3 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 0.1, 0.1
    elseif hp_ratio <= 0.6 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 1.0, 0.1
    end
    
    -- HP 바 배경
    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + bar_width, ui_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )
    
    -- HP 바 채우기
    local hp_fill_width = bar_width * hp_ratio
    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + hp_fill_width, ui_y + bar_height,
        hp_c_r, hp_c_g, hp_c_b, 1.0,
        true
    )
    
    -- HP 바 테두리
    local bar_end_x = ui_x + bar_width
    local bar_end_y = ui_y + bar_height
    DebugDraw.Line(ui_x, ui_y, bar_end_x, ui_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, bar_end_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, ui_y, ui_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, ui_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    
    -- HP 텍스트
    local hp_text = string.format("HP: %d / %d", currentHP, MaxHP)
    DebugDraw.Text(
        hp_text,
        ui_x, ui_y, bar_end_x, bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        14.0, true, true, "Consolas"
    )
    
    -- ============ LIGHT EXPOSURE BAR ============
    local light_bar_y = ui_y + bar_height + bar_spacing
    local exposure_ratio = CurrentLightExposureTime / MaxLightExposureTime
    
    -- Light 색상 결정 (시간이 적을수록 빨갛게)
    local light_c_r, light_c_g, light_c_b = 0.1, 0.5, 1.0  -- 기본: 파란색
    if exposure_ratio <= 0.2 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.0, 0.0  -- 위험: 빨간색
    elseif exposure_ratio <= 0.5 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.5, 0.0  -- 경고: 주황색
    end
    
    -- Light 바 배경
    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + bar_width, light_bar_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )
    
    -- Light 바 채우기
    local light_fill_width = bar_width * exposure_ratio
    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + light_fill_width, light_bar_y + bar_height,
        light_c_r, light_c_g, light_c_b, 1.0,
        true
    )
    
    -- Light 바 테두리
    local light_bar_end_y = light_bar_y + bar_height
    DebugDraw.Line(ui_x, light_bar_y, bar_end_x, light_bar_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_end_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_y, ui_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, light_bar_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    
    -- Light 텍스트
    local light_text = string.format("EXPOSURE: %.1fs", CurrentLightExposureTime)
    DebugDraw.Text(
        light_text,
        ui_x, light_bar_y, bar_end_x, light_bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        12.0, true, true, "Consolas"
    )
    
    -- 경고 표시 (노출 시간 < 1초)
    if CurrentLightExposureTime < 1.0 then
        local pulse = math.abs(math.sin(totalTime * 5.0))
        DebugDraw.Text(
            "! DANGER !",
            ui_x + bar_width + 15.0, light_bar_y, ui_x + bar_width + 150.0, light_bar_end_y,
            1.0, 0.0, 0.0, 0.5 + pulse * 0.5,
            14.0, true, false, "Arial"
        )
    end
end

function StartGame()
    bStarted = true
    currentHP = MaxHP
    CurrentLightExposureTime = MaxLightExposureTime
    currentRotation = Owner.Rotation:ToEuler()

    Log(string.format("Player.lua BeginPlay. Owner: %s, HP: %d", Owner.UUID, currentHP))
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
    bStarted = false
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
    elseif otherActor.Tag == CollisionTag.Score then
        Log("Score!")
        --GetWorld():DestroyActor(otherActor)
    elseif otherActor.Tag == CollisionTag.Clear then
        Log("Clear!")
        --GetWorld():DestroyActor(otherActor)
    end
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
---
function OnActorEndOverlap(overlappedActor, otherActor)
    -- Log("Overlap ended with: " .. otherActor.Name)
end

---
-- [Enemy Spawning] EnemySpawner에게 스폰 요청
---
function RequestSpawnEnemy()
    local world = GetWorld()
    if world then
        local level = world:GetLevel()
        if level then
            local actor = level:FindActorByName("EnemySpawner")
            if actor then
                local spawner = actor:ToAEnemySpawnerActor()
                if spawner then
                    spawner:RequestSpawn()
                    -- 스폰 타이머는 EnemySpawner가 관리
                end
            end
        end
    end
end