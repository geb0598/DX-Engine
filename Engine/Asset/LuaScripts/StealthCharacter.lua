------------------------------------------------------------------
-- StealthCharacter.lua
-- 스텔스 게임 캐릭터 예시 - 빛 감지 기반 은신
------------------------------------------------------------------
-- [게임 로직]
-- - 어두운 곳 (Luminance < 0.3): 은신 성공, 안전
-- - 밝은 곳 (Luminance > 0.7): 노출됨, 위험
-- - 급격히 밝아짐: 경고음 재생
------------------------------------------------------------------

-- 은신 상태
local bIsHidden = false
local hiddenThreshold = 0.3
local exposedThreshold = 0.7
local dangerFlashSpeed = 2.0
local warningTimer = 0.0

function BeginPlay()
    Log("Stealth Character Initialized")
    bIsHidden = true
end

function Tick(dt)
    -- 노출 상태일 때 경고 효과
    if not bIsHidden then
        warningTimer = warningTimer + dt

        -- 경고 표시 깜빡임 (실제로는 머티리얼 파라미터 변경)
        local flash = math.sin(warningTimer * dangerFlashSpeed * math.pi)
        if flash > 0 then
            -- SetMaterialParameter("EmissiveColor", FLinearColor(1, 0, 0, 1))
        end
    end
end

---
-- 조명 세기 변화 감지
---
function OnLightIntensityChanged(current, previous)
    -- 어두운 곳으로 이동 - 은신 성공
    if current < hiddenThreshold and not bIsHidden then
        bIsHidden = true
        warningTimer = 0.0
        Log("✓ HIDDEN - Entered shadows")
        -- PlaySound("stealth_hide")
        -- SetMaterialParameter("EmissiveColor", FLinearColor(0, 0, 0, 0))

    -- 밝은 곳으로 이동 - 노출됨
    elseif current > exposedThreshold and bIsHidden then
        bIsHidden = false
        Log("✗ EXPOSED - Too bright, enemies may see you!")
        -- PlaySound("stealth_warning")

    -- 급격한 조명 변화 감지 (적의 손전등 등)
    elseif (current - previous) > 0.4 then
        Log("⚠ SUDDEN LIGHT - Flashlight detected!")
        -- PlaySound("stealth_alert")
    end

    -- 현재 위험도 계산 (0.0 ~ 1.0)
    local dangerLevel = math.max(0, math.min(1, (current - hiddenThreshold) / (exposedThreshold - hiddenThreshold)))
    -- UpdateDangerUI(dangerLevel)
end

function EndPlay()
    Log("Stealth Character Destroyed")
end
