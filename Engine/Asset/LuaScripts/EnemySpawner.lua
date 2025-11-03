------------------------------------------------------------------
-- EnemySpawner.lua
-- AEnemySpawnerActor의 Delegate를 통해 Enemy를 스폰하는 스크립트
-- 사용: AEnemySpawnerActor에 ScriptComponent를 추가하고 이 스크립트 할당
------------------------------------------------------------------

local templateEnemy = nil

-- 설정
local spawnOffsetRange = 50            -- 스폰 오프셋 범위 (±값)
local spawnDelay = 2.0                 -- 스폰 간격 (초)
local spawnTimer = 0.0                 -- 스폰 타이머
local maxEnemies = 10                  -- 최대 Enemy 개수

-- 생성된 Enemy 추적 (WeakAActor 사용)
local spawnedEnemies = {}              -- WeakAActor 배열

-- 무효한 Enemy들을 배열에서 제거
local function CleanupInvalidEnemies()
    local validEnemies = {}
    local removedCount = 0

    for i, weakEnemy in ipairs(spawnedEnemies) do
        if weakEnemy:IsValid() then
            table.insert(validEnemies, weakEnemy)
        else
            removedCount = removedCount + 1
        end
    end

    spawnedEnemies = validEnemies

    if removedCount > 0 then
        Log("[EnemySpawner] Cleaned up " .. tostring(removedCount) .. " invalid enemies")
    end
end

function BeginPlay()
    -- 템플릿 액터 캐시
    local world = GetWorld()
    if world then
        -- Template Actor는 Editor World에만 존재하므로
        local editorWorld = world:GetSourceEditorWorld()
        if not editorWorld then
            -- Editor World인 경우 그대로 사용
            editorWorld = world
        end

        local level = editorWorld:GetLevel()
        if level then
            local found = level:FindTemplateActorByName("Enemy")
            if found ~= nil then
                templateEnemy = found
                Log("[EnemySpawner] Cached template 'Enemy'")
            else
                Log("[EnemySpawner] ERROR: Could not find template actor named 'Enemy'")
            end
        end
        -- Owner의 Delegate는 ScriptComponent에서 자동으로 바인딩됨
        -- OnEnemySpawnRequested 함수가 있으면 자동으로 연결됨
        Log("[EnemySpawner] Ready for spawn requests")
    end

end

-- Owner의 OnEnemySpawnRequested Delegate가 호출할 콜백
function OnEnemySpawnRequested()
    -- 타이머 체크: 쿨다운 중이면 스폰하지 않음
    if spawnTimer > 0 then
        return  -- 아직 스폰 가능 시간이 아님
    end

    -- 무효한 Enemy 정리
    CleanupInvalidEnemies()

    -- 현재 살아있는 Enemy 개수 확인
    local currentEnemyCount = #spawnedEnemies

    if currentEnemyCount >= maxEnemies then
        Log("[EnemySpawner] Max enemy limit reached (" .. tostring(currentEnemyCount) .. "/" .. tostring(maxEnemies) .. "), spawn skipped")
        return
    end

    Log("[EnemySpawner] Spawn request received - spawning enemy (" .. tostring(currentEnemyCount + 1) .. "/" .. tostring(maxEnemies) .. ")")

    -- Spawner 자신의 위치 기준으로 랜덤 스폰
    local newEnemy = SpawnInternal(nil)

    -- WeakAActor로 래핑하여 배열에 추가 (안전한 추적)
    if newEnemy ~= nil then
        local weakEnemy = MakeWeakAActor(newEnemy)
        table.insert(spawnedEnemies, weakEnemy)
    end

    -- 타이머 리셋
    spawnTimer = spawnDelay
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
    -- 스폰 타이머 감소
    if spawnTimer > 0 then
        spawnTimer = spawnTimer - dt
        if spawnTimer < 0 then
            spawnTimer = 0
        end
    end

    -- 주기적으로 무효한 Enemy 정리 (매 프레임 체크는 비효율적이므로 타이머와 함께)
    -- spawnTimer가 0이 될 때마다 정리 (즉, 스폰 간격마다)
end

function EndPlay()
    -- 필요 시 정리 (전역 API 제거는 생략: 핫리로드/재시작 시 최신 함수로 덮임)
end

