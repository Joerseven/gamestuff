//
// Created by c3042750 on 01/12/2023.
//

#ifndef CSC8503_CLIENTGAME_H
#define CSC8503_CLIENTGAME_H

#include <iostream>
#include "GameTechRenderer.h"
#include "Window.h"
#include "KeyboardMouseController.h"
#include "NetworkBase.h"
#include "RenderObject.h"
#include "GameClient.h"
#include "Replicated.h"
#include "NetworkObject.h"

using namespace NCL;
using namespace CSC8503;

class ClientGame : PacketReceiver {
public:
    ClientGame();
    ~ClientGame();

    void UpdateGame(float dt);
    void ReceivePacket(int type, GamePacket* payload, int source) override;
protected:
    void InitialiseAssets();
    void InitCamera();
    void UpdateKeys();
    void InitWorld();

    GameTechRenderer* renderer;
    GameWorld* world;
    KeyboardMouseController controller;
    GameClient* thisClient;

    Mesh *cubeMesh = nullptr;
    Mesh *sphereMesh = nullptr;
    Mesh *charMesh = nullptr;

    Texture* basicTex = nullptr;
    Texture* assetColorMap = nullptr;
    Shader* basicShader = nullptr;

    GameObject *AddFloorToWorld(const Vector3& position);
    void AddPlayerObjects(const Vector3& position);

    void StartAsClient(char a, char b, char c, char d);
    std::vector<NetworkObject*> netObjects;
};


#endif //CSC8503_CLIENTGAME_H
