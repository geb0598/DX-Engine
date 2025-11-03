------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local bStarted = false
local bGameEnded = false

-- [Cached References] - BeginPlay에서 초기화
local cachedWorld = nil
local cachedLevel = nil
local cachedEnemySpawner = nil

-- [Movement]
local moveSpeed = 100.0
local rotationSpeed = 30.0
local currentRotation = FVector(0, 0, 0)
local InitLocation = FVector(0, 0, 0)

-- [Health]
local MaxHP = 5
local currentHP = 0
local gameMode = nil

-- [Light Exposure]
local LightCriticalPoint = 1.0
local MaxLightExposureTime = 3.0
local CurrentLightExposureTime = 3.0
local bLightWarningShown = false
local CurrentLightLevel = 0.0

-- [Time & Score]
local MaxTime = 180.0  -- 제한 시간 (3분)
local remainingTime = 180.0
local finalScore = 0

---
-- [Health] HP가 0 이하가 되었는지 확인하고 GameMode의 EndGame을 호출
---
local function CheckForDeath()
    if currentHP <= 0 then
        if gameMode and gameMode.IsGameRunning then
            Log("PlayerHealth: HP is 0. Calling EndGame.")
            EndGameSequence()
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

---
-- [Game] 게임 종료 시퀀스
---
function EndGameSequence()
    bGameEnded = true
    bStarted = false
    
    -- 점수 계산: 남은 시간이 많을수록 높은 점수
    -- 남은 시간 1초당 100점
    finalScore = math.floor(remainingTime * 100)
    
    Log(string.format("Game Ended! Remaining Time: %.2fs, Score: %d", remainingTime, finalScore))
    
    if gameMode then
        gameMode:EndGame()
    end
end

local function EndedTest()
    Log("GameEnded Delegate!")
end

function OnLightIntensityChanged(current, previous)
    CurrentLightLevel = current
    
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
    bGameEnded = false

    -- [Cached References] 초기화
    cachedWorld = GetWorld()
    if cachedWorld then
        cachedLevel = cachedWorld:GetLevel()
        gameMode = cachedWorld:GetGameMode()

        -- EnemySpawner 캐싱
        if cachedLevel then
            local spawnerActor = cachedLevel:FindActorByName("EnemySpawner")
            if spawnerActor then
                cachedEnemySpawner = spawnerActor:ToAEnemySpawnerActor()
                if cachedEnemySpawner then
                    Log("[Player] Cached EnemySpawner")
                else
                    Log("[Player] WARNING: EnemySpawner cast failed")
                end
            else
                Log("[Player] WARNING: EnemySpawner not found")
            end
        end

        gameMode.OnGameStarted = StartGame
        gameMode.OnGameEnded = EndedTest
        gameMode:StartGame()
    end
    StartGame()
end

---
-- 매 프레임 호출됩니다.
---
function Tick(dt)
    -- 게임 종료 화면
    if bGameEnded then
        DrawGameOverUI()
        
        -- Space 키로 재시작
        if IsKeyDown(Keys.Space) then
            RestartGame()
        end
        return
    end
    
    -- 게임 진행 중
    if bStarted == false then
        return
    end

    -- 제한 시간 감소
    remainingTime = remainingTime - dt
    
    -- 시간 초과 체크
    if remainingTime <= 0 then
        remainingTime = 0
        Log("Time Over! Game Failed.")
        EndGameSequence()
        return
    end
    
    Movement(dt)
    UpdateLightExposure(dt)
    DrawDangerOverlay()  -- 위험 오버레이 (먼저 그리기)
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
    MoveDirection.Z = 0

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
-- [UI] 위험 상황 전체 화면 오버레이
---
function DrawDangerOverlay()
    -- Light Critical Point를 넘어섰을 때만 표시
    if CurrentLightLevel < LightCriticalPoint then
        return
    end
    
    -- 빠르게 깜빡이는 효과
    local pulse = math.abs(math.sin(remainingTime * 8.0))
    
    -- 노출 시간이 적을수록 더 강하게 깜빡임
    local intensity = 0.0
    if CurrentLightExposureTime < MaxLightExposureTime then
        local danger_ratio = 1.0 - (CurrentLightExposureTime / MaxLightExposureTime)
        intensity = danger_ratio * 0.4  -- 최대 40% 불투명도
    end
    
    -- 화면 전체를 덮는 빨간 오버레이
    if intensity > 0.05 then
        local alpha = intensity * pulse
        local screen_w = 1920.0
        local screen_h = 1080.0
        
        DebugDraw.Rectangle(
            0, 0, screen_w, screen_h,
            1.0, 0.0, 0.0, alpha,  -- 빨간색, 가변 투명도
            true
        )
    end
end

---
-- [Light Exposure] 매 프레임 빛 노출 시간 업데이트
---
function UpdateLightExposure(dt)
    if CurrentLightLevel >= LightCriticalPoint then
        CurrentLightExposureTime = CurrentLightExposureTime - dt

        if CurrentLightExposureTime < 0 then
            CurrentLightExposureTime = 0
        end

        if CurrentLightExposureTime <= 0 then
            RequestSpawnEnemy()

            if not bLightWarningShown then
                Log("WARNING: Light Exposure Time has reached 0! Spawning enemies!")
                bLightWarningShown = true
            end
        end
    else
        CurrentLightExposureTime = CurrentLightExposureTime + dt

        if CurrentLightExposureTime > MaxLightExposureTime then
            CurrentLightExposureTime = MaxLightExposureTime
        end

        if CurrentLightExposureTime > 0 then
            bLightWarningShown = false
        end
    end
end

---
-- [UI] 게임 플레이 중 UI
---
function DrawUI()
    local ui_x = 80.0
    local ui_y = 60.0
    local bar_width = 200.0
    local bar_height = 20.0
    local bar_spacing = 10.0
    
    -- ============ TIME DISPLAY ============
    local time_y = ui_y - 30.0
    
    -- 남은 시간을 분:초 형식으로 변환
    local minutes = math.floor(remainingTime / 60)
    local seconds = math.floor(remainingTime % 60)
    local time_text = string.format("남은 시간: %d:%02d", minutes, seconds)
    
    -- 시간이 30초 미만이면 빨간색으로 경고
    local time_r, time_g, time_b = 1.0, 1.0, 0.3
    if remainingTime < 30.0 then
        local time_pulse = 0.5 + 0.5 * math.abs(math.sin(remainingTime * 5.0))
        time_r, time_g, time_b = 1.0, time_pulse * 0.3, 0.0
    end
    
    DebugDraw.Text(
        time_text,
        ui_x, time_y, ui_x + bar_width, time_y + 25.0,
        time_r, time_g, time_b, 1.0,
        16.0, true, false, "Consolas"
    )
    
    -- ============ HP BAR ============
    local hp_ratio = currentHP / MaxHP
    
    local hp_c_r, hp_c_g, hp_c_b = 0.1, 1.0, 0.1
    if hp_ratio <= 0.3 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 0.1, 0.1
    elseif hp_ratio <= 0.6 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 1.0, 0.1
    end
    
    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + bar_width, ui_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )
    
    local hp_fill_width = bar_width * hp_ratio
    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + hp_fill_width, ui_y + bar_height,
        hp_c_r, hp_c_g, hp_c_b, 1.0,
        true
    )
    
    local bar_end_x = ui_x + bar_width
    local bar_end_y = ui_y + bar_height
    DebugDraw.Line(ui_x, ui_y, bar_end_x, ui_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, bar_end_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, ui_y, ui_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, ui_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    
    local hp_text = string.format("체력: %d / %d", currentHP, MaxHP)
    DebugDraw.Text(
        hp_text,
        ui_x, ui_y, bar_end_x, bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        14.0, true, true, "Consolas"
    )
    
    -- ============ LIGHT EXPOSURE BAR ============
    local light_bar_y = ui_y + bar_height + bar_spacing
    local exposure_ratio = CurrentLightExposureTime / MaxLightExposureTime
    
    local light_c_r, light_c_g, light_c_b = 0.1, 0.5, 1.0
    if exposure_ratio <= 0.2 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.0, 0.0
    elseif exposure_ratio <= 0.5 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.5, 0.0
    end
    
    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + bar_width, light_bar_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )
    
    local light_fill_width = bar_width * exposure_ratio
    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + light_fill_width, light_bar_y + bar_height,
        light_c_r, light_c_g, light_c_b, 1.0,
        true
    )
    
    local light_bar_end_y = light_bar_y + bar_height
    DebugDraw.Line(ui_x, light_bar_y, bar_end_x, light_bar_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_end_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_y, ui_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, light_bar_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    
    local light_text = string.format("빛 노출 가능 시간: %.1fs", CurrentLightExposureTime)
    DebugDraw.Text(
        light_text,
        ui_x, light_bar_y, bar_end_x, light_bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        12.0, true, true, "Consolas"
    )
    
    if CurrentLightExposureTime < 1.0 then
        local pulse = math.abs(math.sin(remainingTime * 5.0))
        DebugDraw.Text(
            "! DANGER !",
            ui_x + bar_width + 15.0, light_bar_y, ui_x + bar_width + 150.0, light_bar_end_y,
            1.0, 0.0, 0.0, 0.5 + pulse * 0.5,
            14.0, true, false, "Arial"
        )
    end
end

---
-- [UI] 게임 오버 화면
---
function DrawGameOverUI()
    -- 화면 전체를 덮는 반투명 배경
    local screen_w = 1920.0
    local screen_h = 1080.0
    
    DebugDraw.Rectangle(
        0, 0, screen_w, screen_h,
        0.0, 0.0, 0.0, 0.7,  -- 어두운 배경
        true
    )
    
    -- 중앙 패널
    local panel_w = 600.0
    local panel_h = 400.0
    local panel_x = (screen_w - panel_w) / 2.0
    local panel_y = (screen_h - panel_h) / 2.0
    
    DebugDraw.Rectangle(
        panel_x, panel_y, panel_x + panel_w, panel_y + panel_h,
        0.1, 0.1, 0.15, 0.95,
        true
    )
    
    -- 패널 테두리 (빛나는 효과)
    local pulse = 0.5 + 0.5 * math.abs(math.sin(remainingTime * 2.0))
    DebugDraw.Line(panel_x, panel_y, panel_x + panel_w, panel_y, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y + panel_h, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y, panel_x, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x + panel_w, panel_y, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    
    -- Title Text
    local title_text = "코치님께 발각되고 말았다..."
    local title_color_r, title_color_g, title_color_b = 1.0, 0.2, 0.2

    local sub_text = "??? : 다음 발제가 왜 궁금해요?"
    local sub_color_r, sub_color_g, sub_color_b = 1.0, 0.0, 0.0
    
    if currentHP > 0 and remainingTime > 0 then
        title_text = "꿈에서 깼다..."
        title_color_r, title_color_g, title_color_b = 0.2, 1.0, 0.2
        sub_text = "일어나보니 발표 시간을 놓쳤다..."
        sub_color_r, sub_color_g, sub_color_b = 0.2, 1.0, 0.2
    end
    
    DebugDraw.Text(
        title_text,
        panel_x, panel_y + 40.0, panel_x + panel_w, panel_y + 100.0,
        title_color_r, title_color_g, title_color_b, 1.0,
        48.0, true, true, "Arial"
    )

    DebugDraw.Text(
        sub_text,
        panel_x, panel_y + 140.0, panel_x + panel_w, panel_y + 140.0,
        sub_color_r, sub_color_g, sub_color_b, 1.0,
        24.0, true, true, "Arial"
    )
    
    -- 시간 표시
    local minutes = math.floor(remainingTime / 60)
    local seconds = math.floor(remainingTime % 60)
    local time_text = string.format("남은 시간: %d:%02d", minutes, seconds)
    DebugDraw.Text(
        time_text,
        panel_x, panel_y + 140.0, panel_x + panel_w, panel_y + 200.0,
        1.0, 1.0, 1.0, 1.0,
        24.0, false, true, "Consolas"
    )
    
    if currentHP > 0 and remainingTime > 0 then
        -- 점수 표시
        local score_text = string.format("점수: %d", finalScore)
        DebugDraw.Text(
            score_text,
            panel_x, panel_y + 200.0, panel_x + panel_w, panel_y + 260.0,
            1.0, 0.84, 0.0, 1.0,  -- 금색
            36.0, true, true, "Arial"
        )
    end
    
    -- 재시작 안내
    local restart_pulse = 0.6 + 0.4 * math.abs(math.sin(remainingTime * 3.0))
    DebugDraw.Text(
        "Press SPACE to Restart",
        panel_x, panel_y + 320.0, panel_x + panel_w, panel_y + 360.0,
        1.0, 1.0, 1.0, restart_pulse,
        20.0, false, true, "Arial"
    )
end

---
-- [Game] 게임 재시작
---
function RestartGame()
    Log("Restarting game...")
    
    -- 게임 상태 초기화
    bGameEnded = false
    bStarted = true
    remainingTime = MaxTime
    finalScore = 0
    
    -- 플레이어 상태 초기화
    currentHP = MaxHP
    CurrentLightExposureTime = MaxLightExposureTime
    bLightWarningShown = false
    
    Owner.Location = InitLocation
    
    if gameMode then
        gameMode:StartGame()
    end
    
    Log("Game restarted successfully!")
end

function StartGame()
    remainingTime = MaxTime
    finalScore = 0
    bStarted = true
    bGameEnded = false
    currentHP = MaxHP
    CurrentLightExposureTime = MaxLightExposureTime
    currentRotation = Owner.Rotation:ToEuler()
    InitLocation = Owner.Location

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

    if otherActor.Tag == CollisionTag.Enemy then
        TakeDamage(1)
    elseif otherActor.Tag == CollisionTag.Clear then
        Log("Clear!")
        EndGameSequence()
    end
end

---
-- [Enemy Spawning] EnemySpawner에게 스폰 요청
---
-- [Enemy Spawn] 캐시된 EnemySpawner를 사용하여 Enemy 스폰 요청
---
function RequestSpawnEnemy()
    if cachedEnemySpawner then
        cachedEnemySpawner:RequestSpawn()
    else
        Log("[Player] WARNING: EnemySpawner not available")
    end
end