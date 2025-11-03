------------------------------------------------------------------
-- EnemySpawner.lua
-- 임계치 도달 시 외부에서 호출 가능한 스포너 스크립트
-- 사용: 레벨에 Spawner용 Actor를 하나 두고 ScriptComponent에 이 스크립트 할당
------------------------------------------------------------------

local templateEnemy = nil

-- 설정
local spawnOffsetRange = 50            -- 스폰 오프셋 범위 (±값)

function BeginPlay()
    -- 템플릿 액터 캐시
    local world = GetWorld()
    local level = world and world:GetLevel() or nil
    if level then
        local found = level:FindTemplateActorByName("Enemy")
        if found ~= nil then
            templateEnemy = found
            Log("[EnemySpawner] Cached template 'Enemy'")
        else
            Log("[EnemySpawner] Could not find template actor named 'Enemy'")
        end
    end

    -- 전역 스포너 API 등록 (다른 스크립트에서 호출)
    SpawnerAPI = SpawnerAPI or {}

    -- 플레이어나 임의 위치 근처에 스폰
    SpawnerAPI.SpawnEnemyAt = function(targetLocation)
        return SpawnInternal(targetLocation)
    end
end

-- 내부 스폰 함수 (targetLocation이 없으면 Spawner 자신의 위치 기준)
function SpawnInternal(targetLocation)
    if templateEnemy == nil then
        Log("[EnemySpawner] Template not ready; spawn skipped")
        return nil
    end

    local world = GetWorld()
    local level = world and world:GetLevel() or nil
    if level == nil then
        Log("[EnemySpawner] Level not available; spawn skipped")
        return nil
    end

    local baseLocation = targetLocation or Owner.Location
    local spawnOffset = FVector(math.random(-spawnOffsetRange, spawnOffsetRange), math.random(-spawnOffsetRange, spawnOffsetRange), 0)
    local spawnLocation = baseLocation + spawnOffset

    local newEnemy = templateEnemy:DuplicateFromTemplate(level, spawnLocation)
    if newEnemy ~= nil then
        Log("[EnemySpawner] Spawned enemy at: " .. tostring(spawnLocation.X) .. ", " .. tostring(spawnLocation.Y))
    else
        Log("[EnemySpawner] DuplicateFromTemplate failed")
    end
    return newEnemy
end

function Tick(dt)
    -- 스포너 자체는 틱에서 특별히 할 일 없음 (외부 호출 대기)
end

function EndPlay()
    -- 필요 시 정리 (전역 API 제거는 생략: 핫리로드/재시작 시 최신 함수로 덮임)
end

