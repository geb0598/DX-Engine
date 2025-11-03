------------------------------------------------------------------
-- Lua 새 기능 테스트 스크립트
-- 테스트하는 기능:
-- 1. GetWorld() - 월드 접근
-- 2. World:SpawnActor() - 액터 생성
-- 3. World:DestroyActor() - 액터 삭제
-- 4. World:GetAllActorsOfClass() - 액터 쿼리
-- 5. World:GetActorByName() - 이름으로 액터 찾기
-- 6. Actor.Rotation - 회전 속성
-- 7. FQuaternion - 쿼터니언 사용
-- 8. FutureMath.Lerp, Clamp - 수학 유틸리티
------------------------------------------------------------------

local spawnedActors = {}
local elapsedTime = 0

function BeginPlay()
    Log("=== Lua 새 기능 테스트 시작 ===")

    -- 1. GetWorld() 테스트
    local world = GetWorld()
    if world then
        Log("✓ GetWorld() 성공")
        Log("  현재 시간:", world:GetTimeSeconds())
    else
        Log("✗ GetWorld() 실패")
        return
    end

    -- 2. SpawnActor() 테스트
    Log("\n--- 액터 생성 테스트 ---")
    local cube1 = world:SpawnActor("ACubeActor")
    if cube1 then
        cube1.Location = FVector(5, 0, 0)
        cube1.Scale = FVector(0.5, 0.5, 0.5)
        table.insert(spawnedActors, cube1)
        Log("✓ Cube 생성 성공 at", cube1.Location.X, cube1.Location.Y, cube1.Location.Z)
    end

    local sphere1 = world:SpawnActor("ASphereActor")
    if sphere1 then
        sphere1.Location = FVector(-5, 0, 0)
        sphere1.Scale = FVector(0.8, 0.8, 0.8)
        table.insert(spawnedActors, sphere1)
        Log("✓ Sphere 생성 성공")
    end

    -- 3. Rotation 테스트
    Log("\n--- 회전 테스트 ---")
    local rotation = FQuaternion.FromEuler(FVector(0, 45, 0))
    if cube1 then
        cube1.Rotation = rotation
        Log("✓ Cube 회전 설정 (Y축 45도)")
    end

    -- 4. GetAllActorsOfClass() 테스트
    Log("\n--- 액터 쿼리 테스트 ---")
    local allCubes = world:GetAllActorsOfClass("ACubeActor")
    Log("✓ 씬의 총 Cube 개수:", #allCubes)

    local allSpheres = world:GetAllActorsOfClass("ASphereActor")
    Log("✓ 씬의 총 Sphere 개수:", #allSpheres)

    -- 5. FutureMath 테스트
    Log("\n--- FutureMath 테스트 ---")
    local lerped = FutureMath.Lerp(0, 10, 0.5)
    Log("✓ Lerp(0, 10, 0.5) =", lerped)

    local clamped = FutureMath.Clamp(15, 0, 10)
    Log("✓ Clamp(15, 0, 10) =", clamped)

    Log("\n=== 테스트 완료, Tick에서 애니메이션 시작 ===")
end

function Tick(dt)
    elapsedTime = elapsedTime + dt

    -- 생성한 액터들을 회전시키기
    for i, actor in ipairs(spawnedActors) do
        if actor then
            -- Y축으로 회전
            local angle = elapsedTime * 50  -- 초당 50도
            local rotation = FQuaternion.FromEuler(FVector(0, angle, 0))
            actor.Rotation = rotation

            -- 위아래로 부드럽게 이동 (Lerp 사용)
            local baseY = 0
            local oscillation = math.sin(elapsedTime * 2) * 2  -- -2 ~ +2 범위
            local targetY = baseY + oscillation

            local currentLoc = actor.Location
            local newY = FutureMath.Lerp(currentLoc.Y, targetY, dt * 2)
            actor.Location = FVector(currentLoc.X, newY, currentLoc.Z)
        end
    end

    -- 10초 후 생성한 액터들 삭제
    if elapsedTime > 10 and #spawnedActors > 0 then
        Log("\n--- 액터 삭제 테스트 (10초 경과) ---")
        local world = GetWorld()
        for i, actor in ipairs(spawnedActors) do
            if world:DestroyActor(actor) then
                Log("✓ 액터 삭제 요청 성공:", actor.Name)
            end
        end
        spawnedActors = {}  -- 배열 비우기
    end
end

function EndPlay()
    Log("=== 테스트 스크립트 종료 ===")
end
