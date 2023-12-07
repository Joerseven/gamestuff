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
#include "lutils.h"
#include <lua.hpp>
#include "TweenManager.h"

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
    void GetClientInput();
    void LoadLevel(lua_State *L, int level);

    void AddObjectFromLua(lua_State *L);
    void AddVolume(GameObject* g, const std::string& volumeType, lua_State *L);

    Texture* GetTexture(const std::string& texture);
    Mesh* GetMesh(const std::string& mesh);
    Shader* GetShader(const std::string& shader);

    int netIdCounter = 0;

    GameTechRenderer* renderer;
    GameWorld* world;
    KeyboardMouseController controller;
    GameClient* thisClient;

    Texture* basicTex = nullptr;
    Texture* assetColorMap = nullptr;
    Shader* basicShader = nullptr;

    std::unordered_map<std::string, Texture*> textures;
    std::unordered_map<std::string, Mesh*> meshes;
    std::unordered_map<std::string, Shader*> shaders;

    TweenManager* tweenManager;

    //GameObject *AddFloorToWorld(const Vector3& position);
    void AddPlayerObjects(const Vector3& position);

    void StartAsClient(char a, char b, char c, char d);
    std::vector<NetworkObject*> netObjects;

    int recievedPackets = 0;
};


#endif //CSC8503_CLIENTGAME_H
