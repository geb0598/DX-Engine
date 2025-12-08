--[[
시작 지점 : (166.79, 1.3, 3)
농구장 통과 후 : (86.0, -108.0, 3)
대로 통과 후 : (71.3, -233.7, 3)
점프 직전 : (88.7, -268.0, 12)
]]

local GameMode = nil
function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID)
    GameMode = GetOwnerAs(Obj, "AGameModeBase")
    GameMode.PlayerSpawnLocation = Vector(166.79, 1.3, 3)
    GameMode.PlayerSpawnRotationEuler = Vector(0, 0, 180.0)
end

function EndPlay()
    print("[EndPlay] " .. Obj.UUID)
end

function OnBeginOverlap(OtherActor)
    --[[Obj:PrintLocation()]]--
end

function OnEndOverlap(OtherActor)
    --[[Obj:PrintLocation()]]--
end

function Tick(dt)
    
    --Obj:GetPlayerController()
    --[[Obj:PrintLocation()]]--
    --[[print("[Tick] ")]]--
end