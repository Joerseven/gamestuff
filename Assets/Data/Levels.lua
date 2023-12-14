require "math"
require "debug"
require "table"

function dump(o)
    if type(o) == 'table' then
        local s = '{ '
        for k,v in pairs(o) do
            if type(k) ~= 'number' then k = '"'..k..'"' end
            s = s .. '['..k..'] = ' .. dump(v) .. ','
        end
        return s .. '} '
    else
        return tostring(o)
    end
end

Vector4 = {}

function Vector4:new(x,y,z,w)
    local v = {}
    setmetatable(v, self)
    v.x = x
    v.y = y
    v.z = z
    v.w = w
    return v
end

Vector4.__tostring = function(vec)
    return vec.x .. vec.y .. vec.z .. vec.w
end

Vector4.__mul = function(vec, scale)
    return Vector4:new(vec.x * scale, vec.y * scale, vec.z * scale, vec.w * scale)
end


Vector3 = {}

function Vector3:new(x, y, z)
    local v = {}
    setmetatable(v, self)
    v.x = x
    v.y = y
    v.z = z
    return v
end

Vector3.__tostring = function(vec)
    return vec.x .. vec.y .. vec.z
end

Vector3.__mul = function(vec, scale)
    local new = Vector3:new(vec.x * scale, vec.y * scale, vec.z * scale)
    return new
end

Vector3.__add = function(vec1, vec2)
    return Vector3:new(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z)
end

local base = {
    name = "",
    mesh = "Cube.msh",
    shader = "scene",
    texture = "none",
    bounding = "SphereVolume",
    mass = "0",
    color = Vector4:new(1.0,1.0,1.0,1.0),
    active = true,
    boundingSize = 1.0,
    network = false,
    modelOffset = -0.5,
    isTrigger = false,
    position = Vector3:new(0, 0, 0),
    size = Vector3:new(1, 1, 1)
}

base.__index = base

local function CreateObject()
    local o = {}
    setmetatable(o, base)
    return o
end

local function CreateFloor(x,y,z)
    local floor = CreateObject()
    floor.size = Vector3:new(5,5, 5)
    floor.boundingSize = floor.size * 0.5
    floor.position = Vector3:new(x, y, z)
    floor.bounding = "AABBVolume"
    floor.name = "floor"
    floor.mesh = "blockRounded.obj"
    return { floor }
end

local function CreateSpawnPoint(x,y,z)

    local floorPartT = CreateFloor(x,y,z)
    local floorPart = floorPartT[1]

    local triggerPoint = CreateObject()
    triggerPoint.mesh = "none"
    triggerPoint.size = Vector3:new(5,5,5)
    triggerPoint.boundingSize = triggerPoint.size * 0.5
    triggerPoint.bounding = "AABBVolume"
    triggerPoint.name = "spawn"
    triggerPoint.position = floorPart.position + Vector3:new(0,5,0)
    triggerPoint.isTrigger = true
    triggerPoint.active = true

    return { triggerPoint, floorPart }

end


local function CreatePickup(x,y,z)
    local pickup = CreateObject()
    pickup.mesh = "coinGold.obj";
    pickup.size = Vector3:new(5, 5, 5)
    pickup.bounding = "SphereVolume"
    pickup.boundingSize = 0.5;
    pickup.position = Vector3:new(x,y + 2 ,z)
    pickup.name = "coin";
    pickup.network = true;
    return { pickup }
end

local function CreateFlag(x,y,z)
    local flag = CreateObject()
    flag.mesh = "flag.obj"
    flag.position = Vector3:new(x,y+5,z)
    flag.size = Vector3:new(5,5,5)
    flag.bounding = "AABBVolume"
    flag.boundingSize = flag.size * 0.5
    flag.network = true
    flag.isTrigger = true
    flag.name = "flag"

    local goalPlatform = CreateObject();
    goalPlatform.mesh = "blockMoving.obj"
    goalPlatform.size = Vector3:new(5,5,5)
    goalPlatform.bounding = "AABBVolume"
    goalPlatform.boundingSize = goalPlatform.size * 0.5
    goalPlatform.boundingSize.y = goalPlatform.boundingSize.y * 0.25
    goalPlatform.network = true
    goalPlatform.position = Vector3:new(x,y+3,z)
    goalPlatform.name = "platform"
    goalPlatform.modelOffset = -0.125
    return { flag, goalPlatform }
end

itemIds = {
    [0] = CreateFloor,
    [50] = CreateSpawnPoint,
    [100] = CreatePickup,
    [200] = CreateFlag,
}

spawnPoints = {
    Vector3:new(0, 10, 0),
    Vector3:new(100, 10, 100),
    Vector3:new(100, 10, 0),
    Vector3:new(0, 10, 100)
}

levels = {
    [1] = {}
}

-- second argument is map ratio, difference between tilemap size and map size
local level = LoadLevelFromImage("leveltest.png", 5)

for k,v in ipairs(level) do
    local thisLevel = levels[1]
    if itemIds[v.r] then
        local tab = itemIds[v.r](v.x, v.g, v.z)
        for _, i in ipairs(tab) do
            thisLevel[#thisLevel + 1] = i
        end
    end
end