//
// Created by jdhyd on 12/2/2023.
//

#ifndef CSC8503_LEVEL_H
#define CSC8503_LEVEL_H

#include "NetworkObject.h"
#include "GameObject.h"
#include "GameWorld.h"
#include "GameTechRenderer.h"
#include "PhysicsSystem.h"
#include "KeyboardMouseController.h"
#include "RenderObject.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

enum NetworkObjectTypes {
    PlayerObject
};

class LevelSystem {
public:
    LevelSystem();
    ~LevelSystem();
    static GameWorld* GetLevelOne();
    GameObject* AddPlayerObject(GameWorld* g, const Vector3& position);
    GameObject* AddFloor(GameWorld* g, const Vector3 position);
private:
    Mesh* cubeMesh = nullptr;
    Mesh* charMesh = nullptr;
    Texture* basicTex = nullptr;
    Shader*	basicShader = nullptr;
};


#endif //CSC8503_LEVEL_H
