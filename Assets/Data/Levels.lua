local Vector3 = {}

function Vector3:new(x, y, z)
    local v = {}
    setmetatable(v, self)
    v.x = x
    v.y = y
    v.z = z
    return v
end

Vector3.__mul = function(vec, scale)
    return Vector3:new(vec.x * scale, vec.y * scale, vec.z * scale)
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
    floor.boundingSize = Vector3:new(200, 2, 200);
    floor.size = floor.boundingSize * 2
    floor.position = Vector3:new(0, -20, 0)
    floor.bounding = "AABBVolume"
    floor.boundingSize = floor.size;
    floor.texture = "checkerboard"
    return floor
end

levels = {
    [1] = {
        CreateFloor(),
    }
}