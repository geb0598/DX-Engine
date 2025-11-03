------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local bStarted = false

-- [Movement]
local moveSpeed = 10.0
local rotationSpeed = 80.0
local currentRotation = FVector(0, 0, 0)

-- [Health]
local MaxHP = 5
local currentHP = 0
local gameMode = nils
local LightIntensity = 0
---
-- [Health] HP가 0 이하가 되었는지 확인하고 GameMode의 EndGame을 호출
---

function OnLightIntensityChanged(current, previous)
    local delta = current - previous

    -- 조명 변화 로그
    Log(string.format("Light Changed: %.3f -> %.3f (Delta: %.3f)",
        previous, current, delta))

    -- 밝기 상태 출력
    if current < 0.2 then
        Log("  Status: DARK - Hidden in shadows")
    elseif current < 0.5 then
        Log("  Status: DIM - Partially visible")
    elseif current < 0.8 then
        Log("  Status: BRIGHT - Clearly visible")
    else
        Log("  Status: VERY BRIGHT - Fully exposed!")
    end
end


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

	Movement(dt)
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
        -- 1. 이번 프레임에 이동할 목표 위치를 계산합니다.
        local targetLocation = Owner.Location + (MoveDirection * moveSpeed * dt)

        -- 2. 월드와 레벨을 가져옵니다.
        local world = GetWorld()
        local level = nil
        if world then
            level = world:GetLevel()
        end

        -- 3. 레벨의 Sweep 함수를 호출하여 'Wall' 태그와 충돌하는지 확인합니다.
        local hitResult = nil
        if level then
            hitResult = level:SweepActorSingle(Owner, targetLocation, CollisionTag.Wall)
        end

        -- 4. hitResult가 nil일 때 (즉, 아무것도 부딪히지 않았을 때)만 이동을 적용합니다.
        if hitResult == nil then
            Owner.Location = targetLocation
        else
            Log("Movement blocked by wall.")
        end
        -------------------------------------------------
    end
    
    --- [Movement] 회전 ---
    local mouseDelta = GetMouseDelta()
    
    currentRotation.X = 0
    local newPitch = currentRotation.Y - (mouseDelta.Y * rotationSpeed * dt)
    currentRotation.Y = Clamp(newPitch, -89.0, 89.0)
    currentRotation.Z = currentRotation.Z + (mouseDelta.X * rotationSpeed * dt) -- Yaw
    
    Owner.Rotation = FQuaternion.FromEuler(currentRotation)
end

function DrawUI()
    local hp_text = string.format("HP: %d", currentHP)

    local text_pos_x = 100.0
    local text_pos_y = 100.0
    local text_width = 300.0
    local text_height = 50.0

    local r_left = text_pos_x
    local r_top = text_pos_y
    local r_right = text_pos_x + text_width
    local r_bottom = text_pos_y + text_height

    local c_r = 0.1
    local c_g = 1.0
    local c_b = 0.1
    local c_a = 1.0

    -- 텍스트 그리기 호출
    DebugDraw.Text(
        hp_text, 
        r_left, r_top, r_right, r_bottom,  -- Rect(l, t, r, b)
        c_r, c_g, c_b, c_a,               -- Color(r, g, b, a)
        20.0, false, false, "Consolas"
    )
end

function StartGame()
	bStarted = true
    currentHP = MaxHP
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