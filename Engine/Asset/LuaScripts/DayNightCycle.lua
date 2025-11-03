------------------------------------------------------------------
-- DayNightCycle.lua
-- 낮/밤 사이클 감지 및 반응
------------------------------------------------------------------
-- [사용 사례]
-- - NPC 행동 변화 (밤에는 집으로, 낮에는 활동)
-- - 몬스터 스폰 (밤에만 스폰)
-- - 상점 개장/폐장 시간
------------------------------------------------------------------

-- 시간대 정의
local TimeOfDay = {
    NIGHT = 0,      -- 완전히 어두움 (< 0.15)
    DAWN = 1,       -- 새벽 (0.15 ~ 0.35)
    DAY = 2,        -- 낮 (0.35 ~ 0.75)
    DUSK = 3        -- 황혼 (0.75 ~ 0.95)
}

-- 상태
local currentTime = TimeOfDay.DAY
local previousTime = TimeOfDay.DAY

function BeginPlay()
    Log("Day/Night Cycle Detector Started")
end

---
-- 조명 세기로 시간대 판단
---
function OnLightIntensityChanged(current, previous)
    previousTime = currentTime

    -- 시간대 판정
    if current < 0.15 then
        currentTime = TimeOfDay.NIGHT
    elseif current < 0.35 then
        currentTime = TimeOfDay.DAWN
    elseif current < 0.75 then
        currentTime = TimeOfDay.DAY
    else
        currentTime = TimeOfDay.DUSK
    end

    -- 시간대 변화 감지
    if currentTime ~= previousTime then
        OnTimeOfDayChanged(previousTime, currentTime)
    end
end

---
-- 시간대 변화 이벤트
---
function OnTimeOfDayChanged(from, to)
    local timeNames = {"NIGHT", "DAWN", "DAY", "DUSK"}
    Log(string.format("⏰ Time Changed: %s -> %s",
        timeNames[from + 1], timeNames[to + 1]))

    -- 낮이 밝아옴
    if to == TimeOfDay.DAWN then
        Log("  🌅 Dawn is breaking - Day activities starting")
        -- SpawnDayNPCs()
        -- DespawnNightMonsters()

    -- 낮
    elseif to == TimeOfDay.DAY then
        Log("  ☀️ Daytime - Full activity")
        -- OpenShops()

    -- 황혼
    elseif to == TimeOfDay.DUSK then
        Log("  🌆 Dusk approaching - Prepare for night")
        -- WarningNPCs()

    -- 밤
    elseif to == TimeOfDay.NIGHT then
        Log("  🌙 Nighttime - Monsters may spawn")
        -- SpawnNightMonsters()
        -- CloseShops()
        -- SendNPCsHome()
    end
end

function Tick(dt)
    -- 시간대별 지속 효과
    if currentTime == TimeOfDay.NIGHT then
        -- 밤에는 스태미나 회복 느림
    elseif currentTime == TimeOfDay.DAY then
        -- 낮에는 스태미나 회복 빠름
    end
end

function EndPlay()
    Log("Day/Night Cycle Detector Stopped")
end
