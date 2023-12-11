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
    floor.size = Vector3:new(10,10, 10);
    floor.boundingSize = floor.size
    floor.position = Vector3:new(x, y, z)
    floor.bounding = "AABBVolume"
    floor.name = "floor"
    floor.mesh = "blockRounded.obj"
    return floor
end

local function CreateWall(x, y, z)
    local wall = CreateObject()
    wall.size = Vector3:new(10, y + 10, 10)
    wall.bounding = "AABBVolume"
    wall.name = "wall"
    wall.boundingSize = wall.size * 0.5;
    wall.position = Vector3:new(x, 5 + y/2, z);
    return wall
end

local function CreatePickup(x,y,z)
    local pickup = CreateObject()
    pickup.mesh = "coinGold.obj";
    pickup.size = Vector3:new(5, 5, 5)
    pickup.bounding = "SphereVolume"
    pickup.boundingSize = 0.5;
    pickup.position = Vector3:new(x,y + 2 ,z)
    pickup.color = Vector4:new(255/255, 195/255, 20/255, 1.0)
    pickup.name = "coin";
    pickup.network = true;
    return pickup
end

spawnPoint = Vector3:new(0, 10, 0)

levels = {
    [1] = {
        CreateFloor(),
    }
}

-- second argument is map ratio, difference between tilemap size and map size
local level = LoadLevelFromImage("leveltest.png", 10)

for k,v in ipairs(level) do
    local thisLevel = levels[1]
    if (v.r == 0) then
        thisLevel[#thisLevel+1] = CreateFloor(v.x, v.g, v.z);
    elseif (v.r == 100) then
        thisLevel[#thisLevel+1] = CreatePickup(v.x, v.g, v.z)
    end
end