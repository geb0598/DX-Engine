------------------------------------------------------------------
-- LightResponsiveObject.lua
-- 빛에 반응하는 오브젝트 예시
------------------------------------------------------------------
-- [사용 사례]
-- - 태양광 패널: 밝을수록 효율 증가
-- - 뱀파이어: 밝은 빛에 데미지
-- - 식물: 빛이 있어야 성장
-- - 조명 센서: 어두워지면 자동으로 라이트 켜기
------------------------------------------------------------------

-- 오브젝트 상태
local currentEnergy = 0.0
local maxEnergy = 100.0
local chargeRate = 10.0  -- 초당 충전량 (Luminance = 1.0 기준)
local isActive = false

function BeginPlay()
    Log("Light Responsive Object Started")
    currentEnergy = 0.0
end

function Tick(dt)
    -- 에너지가 충분하면 동작
    if currentEnergy > 50.0 and not isActive then
        isActive = true
        Log("✓ System ACTIVATED - Sufficient energy")
        -- EnableFunctionality()
    elseif currentEnergy < 20.0 and isActive then
        isActive = false
        Log("✗ System DEACTIVATED - Low energy")
        -- DisableFunctionality()
    end

    -- 에너지 자연 감소
    currentEnergy = math.max(0, currentEnergy - 2.0 * dt)
end

---
-- 조명에 따라 에너지 충전
---
function OnLightIntensityChanged(current, previous)
    -- 밝기에 비례하여 에너지 충전
    -- (실제로는 Tick에서 current를 사용해야 하지만 예시용)
    local chargeAmount = current * chargeRate * 0.1  -- 0.1초 간격으로 호출된다고 가정
    currentEnergy = math.min(maxEnergy, currentEnergy + chargeAmount)

    -- 로그 출력
    local percentage = (currentEnergy / maxEnergy) * 100
    Log(string.format("Light: %.2f | Energy: %.1f%% | Status: %s",
        current, percentage, isActive and "ACTIVE" or "STANDBY"))

    -- 임계값 알림
    if current > 0.8 then
        Log("  ⚡ High intensity - Fast charging!")
    elseif current < 0.2 then
        Log("  ⚠ Low light - Minimal charging")
    end
end

function EndPlay()
    Log("Light Responsive Object Destroyed")
end
