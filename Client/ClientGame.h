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
#include "PushdownMachine.h"
#include "PushdownState.h"

using namespace NCL;
using namespace CSC8503;

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    Vector2i  Size;       // Size of glyph
    Vector2i  Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};



class ClientGame : PacketReceiver {
public:
    ClientGame();
    ~ClientGame();

    void UpdateGame(float dt);
    void ReceivePacket(int type, GamePacket* payload, int source) override;
    void Disconnect();
    void GetClientInput();
    void RenderScore();

    GameClient* thisClient;
    SenderAcknowledger<GameClient>* senderAcknowledger;
    int startGame = 0;
    GameTechRenderer* renderer;

protected:
    void InitialiseAssets();
    void UpdateCamera();
    void InitCamera();
    void UpdateKeys();
    void InitWorld();
    void LoadLevel(lua_State *L, int level);
    void UpdateCharacterDirection();

    void AddObjectFromLua(lua_State *L);
    void AddVolume(GameObject* g, const std::string& volumeType, lua_State *L);

    Texture* GetTexture(const std::string& texture);
    Mesh* GetMesh(const std::string& mesh, float offset = -0.5f);
    Shader* GetShader(const std::string& shader);

    int netIdCounter = 0;
    GameWorld* world;
    KeyboardMouseController controller;


    Texture* basicTex = nullptr;
    Texture* assetColorMap = nullptr;
    Shader* basicShader = nullptr;

    PushdownMachine* stateManager;

    std::unordered_map<std::string, Texture*> textures;
    std::unordered_map<std::string, Mesh*> meshes;
    std::unordered_map<std::string, Shader*> shaders;

    const Vector3 lockedOffset		= Vector3(0, 8, 20);

    TweenManager* tweenManager;

    GameObject* lockedObject;


    RecieverAcknowledger<GameClient>* recieverAcknowledger;

    //GameObject *AddFloorToWorld(const Vector3& position);
    void AddPlayerObjects(const Vector3& position);

    void StartAsClient(char a, char b, char c, char d);
    std::vector<NetworkObject*> netObjects;

    int recievedPackets = 0;
    float pitch = 0.0f;
    float yaw = 0.0f;

    PlayerScores playerScores {};

    void SetScore(PlayerScores scores);
};

class ReadyScreen : public PushdownState {
public:
    ReadyScreen(ClientGame* g);
    PushdownResult OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) override;
    void OnAwake() override;
    void OnSleep() override;

protected:
    bool isReady;
    ClientGame* g;
};

class PlayScreen : public PushdownState {
public:
    PlayScreen(ClientGame* g);
    PushdownResult OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) override;
    void OnAwake() override;
    void OnSleep() override;
protected:
    ClientGame* g;
};

#endif //CSC8503_CLIENTGAME_H
