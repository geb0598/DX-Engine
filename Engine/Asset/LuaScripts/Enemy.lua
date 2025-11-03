------------------------------------------------------------------
-- Enemy.lua
-- Enemy Actor의 기본 동작 스크립트 (Player 추적)
-- 사용: Enemy 템플릿 Actor에 ScriptComponent를 추가하고 이 스크립트 할당
------------------------------------------------------------------

local moveSpeed = 0.0  -- BeginPlay에서 랜덤 설정
local targetPlayer = nil

-- Jump settings
local initialZ = 0.0
local jumpTime = 0.0
local jumpFrequency = 2.0  -- 점프 주기 (초)
local jumpHeight = 30.0    -- 점프 높이

---
-- Enemy가 생성될 때 호출됩니다.
---
function BeginPlay()
    -- 각 Enemy마다 다른 속도 설정 (15~25 사이)
    moveSpeed = 15.0 + math.random() * 10.0

    Log("[Enemy] Spawned at: " .. tostring(Owner.Location.X) .. ", " .. tostring(Owner.Location.Y) .. " with speed: " .. tostring(moveSpeed))

    -- 초기 Z 위치 저장
    initialZ = Owner.Location.Z

    -- Player 찾기
    local world = GetWorld()
    local level = world and world:GetLevel() or nil
    if level then
        targetPlayer = level:FindActorsOfClass("APlayer")[1]
        if targetPlayer then
            Log("[Enemy] Target player found")
        else
            Log("[Enemy] WARNING: Player not found")
        end
    end
end

---
-- 매 프레임 호출됩니다.
---
function Tick(dt)
    -- 간단한 추적 AI
    if targetPlayer then
        local direction = targetPlayer.Location - Owner.Location
        direction.Z = 0  -- 수평 방향 계산

        if direction:Length() > 1.0 then
            direction:Normalize()

            -- 이동
            local newLocation = Owner.Location + (direction * moveSpeed * dt)

            -- 점프 효과 (Sin 곡선)
            jumpTime = jumpTime + dt
            local jumpOffset = math.abs(math.sin(jumpTime * math.pi * jumpFrequency)) * jumpHeight
            newLocation.Z = initialZ + jumpOffset

            Owner.Location = newLocation

            -- Player 방향을 바라보도록 회전
            Owner.Rotation = FQuaternion.MakeFromDirection(direction)
        end
    end
end

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    Log("[Enemy] Overlap with: " .. otherActor.Name)

    -- Player와 충돌 시 처리
    if otherActor.Tag == CollisionTag.Player then
        Log("[Enemy] Hit player!")
        -- 여기에 플레이어에게 데미지를 주는 로직 추가 가능
    end
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
---
function OnActorEndOverlap(overlappedActor, otherActor)
    -- 필요시 구현
end

---
-- Enemy가 파괴될 때 호출됩니다.
---
function EndPlay()
    Log("[Enemy] EndPlay")
    targetPlayer = nil
end
