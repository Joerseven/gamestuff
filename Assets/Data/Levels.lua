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
    mesh = "Cube",
    shader = "scene",
    texture = "none",
    bounding = "SphereVolume",
    mass = "0",
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

local function CreateFloor()
    local floor = CreateObject()
    floor.size = Vector3:new(200, 2, 200);
    floor.boundingSize = floor.size * 0.5
    floor.position = Vector3:new(-5, 0, -5)
    floor.bounding = "AABBVolume"
    floor.texture = "checkerboard"
    return floor
end

local function CreateWall(x, y, z)
    local wall = CreateObject()
    wall.size = Vector3:new(10, y + 10, 10)
    wall.bounding = "AABBVolume"
    wall.boundingSize = wall.size * 0.5;
    wall.position = Vector3:new(x, 5 + y/2, z);
    return wall
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
        thisLevel[#thisLevel+1] = CreateWall(v.x, v.g, v.z);
    end
end