------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

-- 액터 생성 예시: 1초마다 랜덤 위치에 StaticMeshActor 생성
local spawnTimer = 0
local spawnInterval = 1.0  -- 1초마다 생성
local spawnRange = 10      -- 생성 범위 (-10 ~ 10)
local spawnedActors = {}   -- 생성된 액터들 추적

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    Log("=== Actor Spawn Example Started ===")
    Log("StaticMeshActor will spawn every " .. spawnInterval .. " second(s)")
end

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    -- 타이머 업데이트
    spawnTimer = spawnTimer + dt

    -- 1초마다 액터 생성
    if spawnTimer >= spawnInterval then
        spawnTimer = spawnTimer - spawnInterval  -- 누적 오차 보정

        -- World 가져오기
        local world = GetWorld()
        if world then
            -- 랜덤 위치 생성 (-10 ~ 10 범위)
            local randomX = (math.random() * 2 - 1) * spawnRange
            local randomY = (math.random() * 2 - 1) * spawnRange
            local randomZ = math.random() * 5  -- 0 ~ 5 높이

            -- StaticMeshActor 생성
            local newActor = world:SpawnActor("AStaticMeshActor")

            if newActor then
                -- 위치 설정
                newActor.Location = FVector(randomX, randomY, randomZ)

                -- 랜덤 스케일 (0.5 ~ 1.5)
                local randomScale = 0.5 + math.random()
                newActor.Scale = FVector(randomScale, randomScale, randomScale)

                -- 랜덤 회전
                local randomRotY = math.random() * 360
                newActor.Rotation = FQuaternion.FromEuler(FVector(0, randomRotY, 0))

                -- 생성된 액터 추적
                table.insert(spawnedActors, newActor)

                Log(string.format("✓ Spawned Actor #%d at (%.2f, %.2f, %.2f) scale=%.2f",
                    #spawnedActors, randomX, randomY, randomZ, randomScale))
            else
                Log("✗ Failed to spawn actor")
            end
        end
    end

    -- 생성된 액터들을 천천히 회전시키기 (선택 사항)
    for i, actor in ipairs(spawnedActors) do
        if actor then
            local currentRot = actor.Rotation
            local euler = currentRot:ToEuler()
            euler.Y = euler.Y + dt * 30  -- 초당 30도 회전
            actor.Rotation = FQuaternion.FromEuler(euler)
        end
    end
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
end

------------------------------------------------------------------
-- [Delegate 이벤트 콜백]
-- Owner Actor의 Delegate가 Broadcast될 때 자동으로 호출됩니다.
-- 필요한 이벤트만 정의하면 됩니다.
------------------------------------------------------------------

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
-- @param overlappedActor (AActor): 오버랩된 액터 (Owner)
-- @param otherActor (AActor): 오버랩을 일으킨 다른 액터
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    Log("Overlap started with: " .. otherActor.Name)
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
-- @param overlappedActor (AActor): 오버랩된 액터 (Owner)
-- @param otherActor (AActor): 오버랩을 종료한 다른 액터
---
function OnActorEndOverlap(overlappedActor, otherActor)
    Log("Overlap ended with: " .. otherActor.Name)
end

---
-- 다른 액터와 충돌(Hit)이 발생했을 때 호출됩니다.
-- NOTE: Fires every frame during contact (Unreal-style)
-- @param hitActor (AActor): 충돌한 액터 (Owner)
-- @param otherActor (AActor): 충돌한 다른 액터
-- @param normalImpulse (FVector): 충돌 시 법선 방향 힘
-- @param hit (FHitResult): 충돌 정보 (위치, 법선 등)
---
function OnActorHit(hitActor, otherActor, normalImpulse, hit)
    -- Log("=== OnActorHit (fires every frame) ===")
    -- Log("  Hit with: " .. otherActor.Name)

    -- -- HitResult 상세 정보
    -- Log(string.format("  Normal: (%.3f, %.3f, %.3f)",
    --     hit.Normal.X, hit.Normal.Y, hit.Normal.Z))
    -- Log(string.format("  PenetrationDepth: %.3f", hit.PenetrationDepth))
    -- Log(string.format("  Location: (%.2f, %.2f, %.2f)",
    --     hit.Location.X, hit.Location.Y, hit.Location.Z))
    -- Log("  bBlockingHit: " .. tostring(hit.bBlockingHit))
end

---
-- 조명 세기가 변경되었을 때 호출됩니다.
-- @param current (float): 현재 측정된 Luminance 값 (0.0 ~ 1.0+)
-- @param previous (float): 이전 측정된 Luminance 값
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
