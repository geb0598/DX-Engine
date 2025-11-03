------------------------------------------------------------------
-- AutomaticLightControl.lua
-- 자동 조명 제어 시스템
------------------------------------------------------------------
-- [기능]
-- - 주변이 어두워지면 자동으로 조명 켜기
-- - 밝아지면 자동으로 조명 끄기
-- - 에너지 절약을 위한 스마트 제어
------------------------------------------------------------------

-- 설정
local autoLightThreshold = 0.4  -- 이보다 어두우면 라이트 켜기
local hysteresis = 0.1  -- 떨림 방지용 여유값
local lightComponent = nil  -- C++에서 바인딩할 LightComponent

-- 상태
local isLightOn = false

function BeginPlay()
    Log("Automatic Light Control System Started")

    -- LightComponent 찾기 (C++ 바인딩 필요)
    -- lightComponent = Owner:GetComponentByClass("UPointLightComponent")

    isLightOn = false
end

---
-- 주변 밝기에 따라 조명 자동 제어
---
function OnLightIntensityChanged(current, previous)
    -- 어두워짐 - 조명 켜기
    if current < (autoLightThreshold - hysteresis) and not isLightOn then
        TurnOnLight()

    -- 밝아짐 - 조명 끄기
    elseif current > (autoLightThreshold + hysteresis) and isLightOn then
        TurnOffLight()
    end
end

---
-- 조명 켜기
---
function TurnOnLight()
    isLightOn = true
    Log("💡 Auto Light: ON (ambient too dark)")

    -- 실제 구현 (C++ 바인딩 필요)
    -- if lightComponent then
    --     lightComponent:SetIntensity(10.0)
    --     lightComponent:SetVisibility(true)
    -- end
end

---
-- 조명 끄기
---
function TurnOffLight()
    isLightOn = false
    Log("💡 Auto Light: OFF (ambient bright enough)")

    -- 실제 구현 (C++ 바인딩 필요)
    -- if lightComponent then
    --     lightComponent:SetVisibility(false)
    -- end
end

function Tick(dt)
    -- 에너지 절약 모드: 주기적으로 체크
end

function EndPlay()
    Log("Automatic Light Control System Stopped")
end
