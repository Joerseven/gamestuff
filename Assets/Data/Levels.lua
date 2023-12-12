require "math"
require "debug"

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
    floor.size = Vector3:new(5,5, 5);
    floor.boundingSize = floor.size * 0.5
    floor.position = Vector3:new(x, y, z)
    floor.bounding = "AABBVolume"
    floor.name = "floor"
    floor.mesh = "blockRounded.obj"
    return floor
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
    return pickup
end

local function CreateGoalPlatform(x,y,z)
    local goalPlatform = CreateObject();
    goalPlatform.mesh = "blockMoving.obj"
    goalPlatform.size = Vector3:new(5,5,5)
    goalPlatform.bounding = "AABBVolume"
    goalPlatform.boundingSize = goalPlatform.size * 0.5
    goalPlatform.boundingSize.y = goalPlatform.boundingSize.y * 0.25
    goalPlatform.network = true
    goalPlatform.position = Vector3:new(x,y,z)
    goalPlatform.name = "movingplatform"
    goalPlatform.modelOffset = -0.125
    return goalPlatform
end

local function CreateFlag(x,y,z)
    local flag = CreateObject()
    flag.mesh = "flag.obj"
    flag.position = Vector3(x,y,z)
    flag.size = Vector3:new(5,5,5)
    flag.bounding = "AABBVolume"
    flag.boundingSize = flag.size * 0.5;
    flag.network = true
    flag.name = "flag"
    return flag
end

itemIds = {
    [0] = CreateFloor,
    [100] = CreatePickup,
    [200] = CreateGoalPlatform,
    [150] = CreateFlag

}

spawnPoints = {
    Vector3:new(0, 10, 0),
    Vector3:new(200, 10, 200),
    Vector3:new(200, 10, 0),
    Vector3:new(0, 10, 200)
}

levels = {
    [1] = {}
}

-- second argument is map ratio, difference between tilemap size and map size
local level = LoadLevelFromImage("leveltest.png", 5)
local level2 = LoadLevelFromImage("levelitems.png", 5)

for k,v in ipairs(level) do
    local thisLevel = levels[1]
    if itemIds[v.r] then
        thisLevel[#thisLevel+1] = itemIds[v.r](v.x, v.g, v.z)
    end
end