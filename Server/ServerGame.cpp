//
// Created by c3042750 on 01/12/2023.
//

#include "ServerGame.h"

GameObject* moreTheFloor;

ServerGame::ServerGame() {

    NetworkBase::Initialise();
    server = new GameServer(NetworkBase::GetDefaultPort(), 4,[&](int peerId) {
        CreatePlayer(peerId);
    },
                            [&](int peerId) {
        KillPlayer(players[playerMap[peerId]]);
        PlayerLeft(peerId);
    });
    server->RegisterPacketHandler(Received_State, this);
    server->RegisterPacketHandler(Server_Message, this);
    server->RegisterPacketHandler(Acknowledge_Packet, this);

    std::cout << "Server starting up" << std::endl;

    world = new GameWorld();
    physics = new PhysicsSystem(*world);

    forceMagnitude = 10.0f;
    timeToNextPacket  = 0.0f;
    netIdCounter = 3; // Players added will bring it up to 3. Why did I do it like this it's like I'm incapable of writing code like a normal human.

    jumpForgiveness = 0;

    stateManager = new PushdownMachine(new ReadyScreenServer(this));

    playerScores.fill(0);

    physics->UseGravity(true);

    ClearPlayers();

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    RegisterFunctions(L);

    auto status = luaL_dofile(L, ASSETROOTLOCATION "Data/Levels.lua");

    if (status) {
        std::cerr << "Lua file giga dead: " << lua_tostring(L, -1);
        exit(1);
    }

    lua_getglobal(L, "spawnPoints");
    for (int i = 0; i < spawnPoints.size(); i++) {
        spawnPoints[i] = getVec3Field(L, i+1);
    }
    lua_pop(L, 1);

    AddPlayerObjects();

    LoadLevel(L, 1);

    InitialiseEvents();

    lua_close(L);

}

ServerGame::~ServerGame() {
    delete physics;
    delete world;
    delete server;
    delete deathObserver;
}

void ServerGame::UpdateGame(float dt) {

    stateManager->Update(dt);

    UpdatePlayers(dt);

    timeToNextPacket -= dt;
    if (timeToNextPacket < 0) {
        BroadcastSnapshot();
        timeToNextPacket += SERVERHERTZ;
        for (auto& m : playerRecievers) {
            m.second->SendAcknowledgement();
        }
        for (auto& m : playerSenders) {
            m.second->CatchupPackets();
        }
    }

    world->UpdateWorld(dt);
    physics->Update(dt);

    LimitPlayerLinearVelocitys();
    CheckOffMapPlayers();


    server->UpdateServer();
}

void ServerGame::InitialiseEvents() {

    deathObserver = new Subject<GameObject*>();

    world->OperateOnContents([this] (GameObject* o){
        if (o->name == "flag") {
            o->collisionListener = new Subject<GameObject*>();
            ListenFlagPickedUp(o);
        }
    });
}

void ServerGame::CheckOffMapPlayers() {
    for (int i=0; i<players.size(); i++) {
        if (players[i]->GetTransform().GetPosition().y < -20) {
            KillPlayer(players[i]);
            deathObserver->Trigger(players[i]);
        }
    }
}

void ServerGame::UpdatePlayers(float dt) {
    for (auto it = playerControls.begin(); it != playerControls.end(); it++) {

        if (playerMap.find(it->first) == playerMap.end()) {
            return;
        }

        auto player = players[playerMap[it->first]];
        auto &pressed = it->second;
        float mag = 10.0f;

        jumpForgiveness -= dt;

        if (CheckPlayerOnGround(player)) {
            jumpForgiveness = 0.5;
        }

        player->GetTransform()
            .SetOrientation(Quaternion::AxisAngleToQuaterion({0, 1, 0}, *((float*)&pressed[1])).Normalised());

        if (!CheckPlayerOnGround(player)) {
            mag = 0.1f;
        }

        player->GetPhysicsObject()->AddForce(player->GetTransform().GetOrientation() * Vector3(0, 0, ((float)pressed[0] * mag * -1) + ((float)pressed[6] * mag)));
    }
}

void ServerGame::BroadcastSnapshot() {
    sentPackets++;
    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;

    world->GetObjectIterators(first, last);

    currentSnapshot++;

    for (auto i = first; i != last; ++i) {

        if (!((*i)->IsActive())) {
            continue;
        }

        NetworkObject* o = (*i)->GetNetworkObject();
        if (!o) {
            continue;
        }

        GamePacket* newPacket = nullptr;
        if (o->WritePacket(&newPacket, false, currentSnapshot)) {
            server->SendGlobalPacket(*newPacket);
            delete newPacket;
        }
    }
}

void DebugPackets(int type, GamePacket *payload, int source) {
    std::cout << "Recieved Packet: " << std::endl;
    std::cout << "type: " << type << std::endl;
    std::cout << "source: " << source << std::endl;
}

GameObject* ServerGame::GetPlayerFromPeer(int peerId) {
    return players[playerMap[peerId]];
}

bool ServerGame::CheckPlayerOnGround(GameObject* player) {

    Ray jumpChecker(player->GetTransform().GetPosition(), Vector3(0, -1, 0));
    RayCollision collisionInfo;

    if (world->Raycast(jumpChecker, collisionInfo, true, player)) {
        auto distance = player->GetTransform().GetPosition().y
                        - collisionInfo.collidedAt.y
                        - ((AABBVolume*)player->GetBoundingVolume())->GetHalfDimensions().y;
        if (distance <= 0.1) {
            return true;
        }
    }

    return false;
}

void ServerGame::PlayerJump(int peerId) {

    auto player = players[playerMap[peerId]];

    if (jumpForgiveness > 0) {
        player->GetPhysicsObject()->AddForce(Vector3(0, 5000, 0));
        jumpForgiveness = 0;
    }
}

void ServerGame::KillPlayer(GameObject* player) {
    // this is terrible but it's nearly wednesday
    auto index = 0;
    for (int i=0; i<players.size(); i++) {
        if (players[i] == player) {
            index = i;
        }
    }
    deathObserver->Trigger(player);
    player->GetPhysicsObject()->ClearForces();
    player->GetPhysicsObject()->SetLinearVelocity(Vector3(0,0,0));
    player->GetTransform().SetPosition(spawnPoints[index]);
}

void ServerGame::ReceivePacket(int type, GamePacket *payload, int source) {

    if (!(playerRecievers[source]->CheckAndUpdateAcknowledged(*payload))) {
        return;
    };

    if (type == Server_Message) {
        auto id = ((ServerMessagePacket*)payload)->messageID;
        if (id == Player_Jump) {
            PlayerJump(source);
            //players[playerMap[source]]->GetPhysicsObject()->AddForce(Vector3(0, 1000, 0));
        }

        if (id == Player_Ready) {
            playerReady[source] = !playerReady[source];
        }
        return;
    }

    if (type == Received_State) {
        auto exists = playerMap.find(source) != playerMap.end();
        if (exists) {
            auto pressed = ((ClientPacket*)payload)->buttonstates;
            float mag = 100.0f;
            memcpy(playerControls[source].data(), pressed, 8*sizeof(char));
        }
        return;
    }

    if (type == Acknowledge_Packet) {
        playerSenders[source]->ReceiveAcknowledgement(((AcknowledgePacket*)payload)->acknowledge);
    }
}


void ServerGame::SetActiveNetworkObject(NetworkObject* networkObject, bool active) {
    for (auto& ps : playerSenders) {
        networkObject->object.SetActive(active);
        FunctionPacket p(SetNetworkObjectActive{networkObject->networkID, networkObject->object.IsActive()}, Functions::SetNetworkObjectActive);
        server->SendPacket(ps.second->RequireAcknowledgement(p), ps.first);
    }
}

void ServerGame::CatchupPlayerJoined(int peerId) {
    for (auto& n : netObjects) {
        FunctionPacket p(SetNetworkObjectActive{n->networkID, n->object.IsActive()}, Functions::SetNetworkObjectActive);
        server->SendPacket(playerSenders[peerId]->RequireAcknowledgement(p), peerId);
    }
}

void ServerGame::ListenFlagDropped(GameObject* playerWhoCaptured, GameObject* flag, Transform transform) {
    auto* playerDeathObserver = new Observer<GameObject*>();
    auto* flagReturnedObserver = new Observer<GameObject*>();

    std::cout << "Flag reset event attached" << std::endl;

    playerDeathObserver->SetOnTrigger([=, this](GameObject* playerWhoDied) {
        if (playerWhoDied == playerWhoCaptured) {

            std::cout << "Flag being put back!" << std::endl;

            flag->GetPhysicsObject()->SetInverseMass(0);

            // This is not a real constraint no marks plz
            world->RemoveConstraint(flagConstraint);
            delete flagConstraint;
            flagConstraint = nullptr;

            flag->GetTransform().SetPosition(transform.GetPosition());
            flag->GetTransform().SetOrientation(transform.GetOrientation());

            flag->collisionListener->Detach(flagReturnedObserver);
            ListenFlagPickedUp(flag);
            return true;
        }

        return false;
    });

    flagReturnedObserver->SetOnTrigger([=, this](GameObject* collidedWith) {
        if (collidedWith->name == "spawn") {

            std::cout << "Player reached spawn with flag!" << std::endl;

            flag->GetPhysicsObject()->SetInverseMass(0);

            world->RemoveConstraint(flagConstraint);
            delete flagConstraint;
            flagConstraint = nullptr;

            flag->GetTransform().SetPosition(transform.GetPosition());
            flag->GetTransform().SetOrientation(transform.GetOrientation());

            for (int i=0; i < players.size(); i++) {
                if (players[i] == playerWhoCaptured) {
                    playerScores[i] += 1;
                    std::cout << "Player got a score!" << std::endl;
                }
            }


            for (auto& sender: playerSenders) {
                PlayerScores p{};
                memcpy(p.values, playerScores.data(), 4 * sizeof(int));
                auto fp = FunctionPacket<PlayerScores>(p, Functions::UpdatePlayerScore);
                server->SendPacket(sender.second->RequireAcknowledgement(fp), sender.first);
            }

            std::cout << "Sent score to players" << std::endl;

            deathObserver->Detach(playerDeathObserver);
            ListenFlagPickedUp(flag);
            return true;
        }
        return false;
    });

    deathObserver->Attach(playerDeathObserver);
    flag->collisionListener->Attach(flagReturnedObserver);
}

void ServerGame::ListenFlagPickedUp(GameObject* flag) {
    Observer<GameObject*>* observer = new Observer<GameObject*>();

    std::cout << "Flag pickup event attached" << std::endl;

    observer->SetOnTrigger([flag, this](GameObject* collidedWith){
        if (collidedWith->name == "player") {

            std::cout << "Flag being picked up!" << std::endl;

            auto originalFlagTransform = flag->GetTransform();

            // This is not a real constraint :(
            flagConstraint = new FixedConstraint(flag, collidedWith, 0, 2, 0.6);
            world->AddConstraint(flagConstraint);
            //flag->GetPhysicsObject()->SetInverseMass(1.0f/2);


            ListenFlagDropped(collidedWith, flag, originalFlagTransform);
            return true;
        }

        return false;
    });

    flag->collisionListener->Attach(observer);
}

void ServerGame::AssignPlayer(NetworkObject* obj, int peerId) {
    FunctionPacket p(AssignPlayerFunction{obj->networkID, peerId}, Functions::AssignPlayerFunction);
    playerSenders[peerId]->RequireAcknowledgement(p);
    server->SendPacket(p, peerId);
}

GameObject *ServerGame::CreatePlayer(int peerId) {

    int freeIndex = 0;

    for (int i=0; i < players.size(); i++) {
        if (players[i]->IsActive()) {
            continue;
        }

        freeIndex = i;
        break;
    }

    playerSenders.insert(std::make_pair(peerId, new SenderAcknowledger(server, peerId)));
    playerRecievers.insert(std::make_pair(peerId, new RecieverAcknowledger(server, peerId)));
    playerMap.insert(std::make_pair(peerId, freeIndex));
    playerReady.insert(std::make_pair(peerId, false));

    SetActiveNetworkObject(players[freeIndex]->GetNetworkObject(), true);
    AssignPlayer(players[freeIndex]->GetNetworkObject(), peerId);
    CatchupPlayerJoined(peerId);

    std::cout << "Player added successfully!" << std::endl;

    return players[freeIndex];
}

void ServerGame::PlayerLeft(int peerId) {

    std::cout << "Leaving rotation is: " << players[playerMap[peerId]]->GetTransform().GetOrientation() << std::endl;

    SetActiveNetworkObject(players[playerMap[peerId]]->GetNetworkObject(), false);

    serverInfo.playerIds[playerMap[peerId]] = -1;
    playerSenders.erase(peerId);
    playerRecievers.erase(peerId);
    playerMap.erase(peerId);

    std::cout << "Player removed successfully" << std::endl;

}

void ServerGame::LimitPlayerLinearVelocitys() {
    for (int i=0; i<players.size(); i++) {
        auto maxSpeed = 10.0f;
        if (!CheckPlayerOnGround(players[i]))
            maxSpeed = 20.0f;

        auto playPhs = players[i]->GetPhysicsObject();
        auto mag = playPhs->GetLinearVelocity().Length();

        if (mag >= maxSpeed) {
            playPhs->SetLinearVelocity(playPhs->GetLinearVelocity().Normalised() * maxSpeed);
        }
    }
}

void ServerGame::LoadLevel(lua_State *L, int level) {
    lua_getglobal(L, "levels");
    lua_pushnumber(L, level);
    lua_gettable(L, -2);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        AddObjectFromLua(L);
        lua_pop(L, 1);
    }
}

void ServerGame::AddObjectFromLua(lua_State *L) {
    // object table is on top of the stack
    GameObject* g = new GameObject(getStringField(L, "name"));
    g->SetActive(getBool(L, "active"));
    Vector3 size = getVec3Field(L, "size");
    Vector3 position = getVec3Field(L, "position");

    g->GetTransform()
            .SetPosition(position)
            .SetScale(size);

    auto volumeType = getStringField(L, "bounding");
    AddVolume(g, volumeType, L);

    if (getBool(L, "network")) {
        g->SetNetworkObject(new NetworkObject(*g, ++netIdCounter));
        netObjects.push_back(g->GetNetworkObject());
    }

    g->GetPhysicsObject()->isTrigger = getBool(L, "isTrigger");

    moreTheFloor = g;

    world->AddGameObject(g);
}

void ServerGame::AddVolume(GameObject* g, const std::string& volumeType, lua_State *L) {
    if ("SphereVolume" == volumeType) {
        auto size = getNumberField(L, "boundingSize");
        auto volume  = new SphereVolume(size);
        g->SetBoundingVolume((CollisionVolume*)volume);
        g->SetPhysicsObject(new PhysicsObject(&g->GetTransform(), g->GetBoundingVolume()));
        g->GetPhysicsObject()->SetInverseMass(getNumberField(L, "mass"));
        g->GetPhysicsObject()->InitSphereInertia();
    }

    if ("AABBVolume" == volumeType) {
        auto size = getVec3Field(L, "boundingSize");
        auto volume = new AABBVolume(size);
        g->SetBoundingVolume((CollisionVolume*)volume);
        g->SetPhysicsObject(new PhysicsObject(&g->GetTransform(), g->GetBoundingVolume()));
        g->GetPhysicsObject()->SetInverseMass(getNumberField(L, "mass"));
        g->GetPhysicsObject()->InitCubeInertia();
    }
}


GameObject* ServerGame::AddFloorToWorld(const Vector3& position) {
    GameObject* floor = new GameObject();

    Vector3 floorSize = Vector3(200, 2, 200);
    AABBVolume* volume = new AABBVolume(floorSize);
    floor->SetBoundingVolume((CollisionVolume*)volume);
    floor->GetTransform()
            .SetScale(floorSize * 2)
            .SetScale(floorSize * 2)
            .SetPosition(position);

    floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

    floor->GetPhysicsObject()->SetInverseMass(0);
    floor->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(floor);

    return floor;
}

void ServerGame::ClearPlayers() {
    for (int i = 0; i < players.size(); i++) {
        players[i] = nullptr;
    }
}

void ServerGame::AddPlayerObjects() {
    for (int i = 0; i < players.size(); i++) {
        float meshSize		= 1.0f;
        float inverseMass	= 0.5f;

        auto character = new GameObject();
        auto volume  = new AABBVolume(Vector3(meshSize * 0.7, meshSize * 1.5, meshSize * 0.7));

        character->SetBoundingVolume((CollisionVolume*)volume);

        character->GetTransform()
                .SetScale(Vector3(meshSize, meshSize, meshSize))
                .SetPosition(spawnPoints[i]);

        character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

        character->GetPhysicsObject()->SetInverseMass(inverseMass);
        character->GetPhysicsObject()->InitSphereInertia();
        character->GetPhysicsObject()->restitutionModifier = 0.0f;

        character->SetNetworkObject(new NetworkObject(*character, i));
        netObjects.push_back(character->GetNetworkObject());

        character->collisionListener = new Subject<GameObject*>();

        character->name = "player";

        character->SetActive(false);

        players[i] = character;
        world->AddGameObject(character);
    }
}


int main() {

    auto timer = GameTimer();
    auto s = new ServerGame();
    timer.Tick();

    while (true) {
        std::this_thread::yield();
        timer.Tick();
        float dt = timer.GetTimeDeltaSeconds();

        if (dt > 0.1f) {
            std::cout << "Skipping large time delta" << std::endl;
            continue; //must have hit a breakpoint or something to have a 1 second frame time!
        }

        s->UpdateGame(dt);
    }

}


PushdownState::PushdownResult ReadyScreenServer::OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) {
    bool allReady = true;

    if (g->playerReady.size() < 1) {
        allReady = false;
    }

    for (const auto& ready : g->playerReady) {
        allReady = allReady && ready.second;
    }

    if (allReady) {
        for (auto& sender : g->playerSenders) {
            MessagePacket p;
            p.messageID = Game_Started;
            g->server->SendPacket(sender.second->RequireAcknowledgement(p), sender.first);
        }
        *pushFunc = new PlayServer(g);
        return PushdownState::Push;
    }

    return PushdownState::NoChange;
}

ReadyScreenServer::ReadyScreenServer(ServerGame *g) {
    this->g = g;
    isReady = false;
}


void ReadyScreenServer::OnAwake() {

}

void ReadyScreenServer::OnSleep() {

}

PlayServer::PlayServer(ServerGame *g) {
    this->g = g;
}

PushdownState::PushdownResult PlayServer::OnUpdate(float dt, NCL::CSC8503::PushdownState **pushFunc) {
    return PushdownState::NoChange;
}

void PlayServer::OnAwake() {

}

void PlayServer::OnSleep() {

}
