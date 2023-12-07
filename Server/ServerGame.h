//
// Created by c3042750 on 01/12/2023.
//

#ifndef CSC8503_SERVERGAME_H
#define CSC8503_SERVERGAME_H

# pragma comment(lib, "winmm.lib") // I have no clue what this does but don't remove it.

#include <iostream>
#include "PhysicsSystem.h"
#include "GameTimer.h"
#include "NetworkBase.h"
#include "GameServer.h"
#include "NetworkObject.h"
#include "PhysicsObject.h"
#include "Replicated.h"
#include "lutils.h"
#include <algorithm>

#include <lua.hpp>

#include <array>

using namespace NCL;
using namespace CSC8503;

class ServerGame : PacketReceiver {
public:
    ServerGame();
    ~ServerGame();

    void UpdateGame(float dt);
    void UpdatePlayers();
    void InitWorld(lua_State *L);
    void ReceivePacket(int type, GamePacket* payload, int source) override;

protected:
    GameTimer timer;
    float timeToNextPacket;
    GameWorld* world;
    PhysicsSystem* physics;
    GameServer* server;


    float forceMagnitude;
    int currentSnapshot;
    int netIdCounter;

    void BroadcastSnapshot();
    void ClearPlayers();

    GameObject *AddFloorToWorld(const Vector3 &position);
    void AddPlayerObjects(const Vector3 &position);
    void AddObjectFromLua(lua_State *L);
    void AddVolume(GameObject* g, const std::string& volumeType, lua_State *L);

    GameObject *CreatePlayer(int peerId);
    void PlayerLeft(int peerId);

    std::array<GameObject*, 4> players;
    std::map<int, int> playerMap;
    std::map<int, std::array<char, 8>> playerControls;
    int playersJoined = 0;

    void LoadLevel(lua_State *L, int level);
};


#endif //CSC8503_SERVERGAME_H
