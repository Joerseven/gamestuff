//
// Created by jdhyd on 12/2/2023.
//

#include "Level.h"



GameWorld *LevelSystem::GetLevelOne() {
    auto world = new GameWorld();
    auto renderer = new GameTechRenderer(*world);
    auto physics = new PhysicsSystem(*world);
    return world;
}

GameObject *LevelSystem::AddPlayerObject(GameWorld* g, const Vector3& position) {
    float meshSize		= 1.0f;
    float inverseMass	= 0.5f;

    GameObject* character = new GameObject();
    SphereVolume* volume  = new SphereVolume(1.0f);

    character->SetBoundingVolume((CollisionVolume*)volume);

    character->GetTransform()
            .SetScale(Vector3(meshSize, meshSize, meshSize))
            .SetPosition(position);

    character->SetRenderObject(new RenderObject(&character->GetTransform(),   charMesh, nullptr, basicShader));
    character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

    character->GetPhysicsObject()->SetInverseMass(inverseMass);
    character->GetPhysicsObject()->InitSphereInertia();

    g->AddGameObject(character);

    return character;
}

GameObject *LevelSystem::AddFloor(GameWorld *g, const Vector3 position) {
    GameObject* floor = new GameObject();

    Vector3 floorSize = Vector3(200, 2, 200);
    AABBVolume* volume = new AABBVolume(floorSize);
    floor->SetBoundingVolume((CollisionVolume*)volume);
    floor->GetTransform()
            .SetScale(floorSize * 2)
            .SetPosition(position);

    floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
    floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

    floor->GetPhysicsObject()->SetInverseMass(0);
    floor->GetPhysicsObject()->InitCubeInertia();

    g->AddGameObject(floor);

    return floor;
}

