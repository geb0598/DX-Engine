local minTime = 2.9          -- seconds (inclusive)
local maxTime = 5.2          -- seconds (inclusive)
local spawnInterval = 0.0
local elapsed = 0.0
local prefabPath = "Data/Prefabs/Bomb.prefab"

local function resetInterval()
    spawnInterval = minTime + (maxTime - minTime) * math.random()
end

function BeginPlay()
    elapsed = 0.0
    resetInterval()
end

function EndPlay()
end

function OnBeginOverlap(OtherActor)
end

function OnEndOverlap(OtherActor)
end

function Tick(dt)
    elapsed = elapsed + dt

    while elapsed >= spawnInterval do
        elapsed = elapsed - spawnInterval

        local bomb = SpawnPrefab(prefabPath)
        if bomb ~= nil then
            bomb.Location = Obj.Location
        end

        resetInterval()
    end
end
