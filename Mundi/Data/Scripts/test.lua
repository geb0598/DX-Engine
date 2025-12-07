local GameMode = nil
function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID)
    GameMode = GetOwnerAs(Obj, "AGameModeBase")
    GameMode.PlayerSpawnLocation = Vector(10,1,10)
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