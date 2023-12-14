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
#include <algorithm>
#include <lua.hpp>
#include <array>
#include "lutils.h"
#include "PositionConstraint.h"
#include "PushdownMachine.h"
#include "PushdownState.h"

using namespace NCL;
using namespace CSC8503;

class ServerGame : PacketReceiver {
public:
    ServerGame();
    ~ServerGame();

    void UpdateGame(float dt);
    void UpdatePlayers(float dt);
    void ReceivePacket(int type, GamePacket* payload, int source) override;

    int gameReady = 0;
    std::unordered_map<int, int> playerMap;
    std::unordered_map<int, bool> playerReady;
    std::unordered_map<int, SenderAcknowledger<GameServer>*> playerSenders;

    GameServer* server;

protected:
    GameTimer timer;
    float timeToNextPacket;
    float serverInfoPacketTime;
    GameWorld* world;
    PhysicsSystem* physics;

    float forceMagnitude;
    int currentSnapshot;
    int netIdCounter;

    PushdownMachine* stateManager;

    void BroadcastSnapshot();
    void ClearPlayers();

    GameObject *AddFloorToWorld(const Vector3 &position);
    void AddPlayerObjects();
    void AddObjectFromLua(lua_State *L);
    void AddVolume(GameObject* g, const std::string& volumeType, lua_State *L);

    GameObject *CreatePlayer(int peerId);
    void PlayerLeft(int peerId);

    float jumpForgiveness;

    std::array<GameObject*, 4> players;
    std::unordered_map<int, RecieverAcknowledger<GameServer>*> playerRecievers;
    std::map<int, std::array<char, 8>> playerControls;
    std::array<int, 4> playerScores;

    std::array<Observer<GameObject*>*,4> pickupFlag;

    Subject<GameObject*>* deathObserver;

    std::vector<NetworkObject*> netObjects;

    FixedConstraint* flagConstraint;

    int playersJoined = 0;

    std::array<Vector3, 4> spawnPoints;

    int sentPackets = 0;

    ServerInfo serverInfo;

    void LoadLevel(lua_State *L, int level);

    void SetActiveNetworkObject(NetworkObject* networkObject, bool active);

    void CatchupPlayerJoined(int peerId);

    void AssignPlayer(NetworkObject *obj, int peerId);
    void PlayerJump(int peerId);

    GameObject *GetPlayerFromPeer(int peerId);

    void KillPlayer(GameObject* player);
    void LimitPlayerLinearVelocitys();
    bool CheckPlayerOnGround(GameObject *player);
    void CheckOffMapPlayers();
    void ListenFlagPickedUp(GameObject *flag);
    void ListenFlagDropped(GameObject *playerWhoCaptured, GameObject *flag, Transform transform);
    void InitialiseEvents();
};

class ReadyScreenServer : public PushdownState {
public:
    ReadyScreenServer(ServerGame* g);
    PushdownResult OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) override;
    void OnAwake() override;
    void OnSleep() override;

protected:
    bool isReady;
    ServerGame* g;
};

class PlayServer : public PushdownState {
public:
    PlayServer(ServerGame *g);
    PushdownResult OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) override;
    void OnAwake() override;
    void OnSleep() override;
protected:
    ServerGame* g;

};


#endif //CSC8503_SERVERGAME_H
