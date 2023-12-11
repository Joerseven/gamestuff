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
    netIdCounter = 3; // Players added will bring it up to 4.

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

    lua_getglobal(L, "spawnPoint");
    auto spawnPoint = Vector3((float)getNumberField(L, "x"), (float)getNumberField(L, "y"), (float)getNumberField(L, "z"));
    lua_pop(L, 1);

    AddPlayerObjects(spawnPoint);

    LoadLevel(L, 1);

    lua_close(L);

}

ServerGame::~ServerGame() {
    delete physics;
    delete world;
    delete server;
}

void ServerGame::UpdateGame(float dt) {

    UpdatePlayers();

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

    server->UpdateServer();
}

void ServerGame::UpdatePlayers() {
    for (auto it = playerControls.begin(); it != playerControls.end(); it++) {

        if (playerMap.find(it->first) == playerMap.end()) {
            return;
        }

        auto player = players[playerMap[it->first]];
        auto &pressed = it->second;
        float mag = 0.2f;

        player->GetTransform()
            .SetOrientation(Quaternion::AxisAngleToQuaterion({0, 1, 0}, *((float*)&pressed[1])).Normalised());

        player->GetPhysicsObject()->AddForce(player->GetTransform().GetOrientation() * Vector3(0, 0, (float)pressed[0] * mag * -1));
//        players[playerMap[it->first]]->GetPhysicsObject()->AddForce(Vector3(
//                (float)pressed[3] * mag + (float)pressed[1] * mag * -1,
//                0,
//                (float)pressed[2] * mag + (float)pressed[0] * mag * -1));
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

void ServerGame::InitWorld(lua_State *L) {
    AddPlayerObjects({0,0,0});

}

void DebugPackets(int type, GamePacket *payload, int source) {
    std::cout << "Recieved Packet: " << std::endl;
    std::cout << "type: " << type << std::endl;
    std::cout << "source: " << source << std::endl;
}

void ServerGame::ReceivePacket(int type, GamePacket *payload, int source) {

    if (!(playerRecievers[source]->CheckAndUpdateAcknowledged(*payload))) {
        return;
    };

    if (type == Server_Message) {
        auto id = ((ServerMessagePacket*)payload)->messageID;
        if (id == Player_Jump) {
            players[playerMap[source]]->GetPhysicsObject()->AddForce(Vector3(0, 1000, 0));
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

void ServerGame::AttachCoinListener(GameObject* player) {
    auto observer = new Observer<GameObject*>([this](GameObject* item){
        if (item->name == "coin") {
            SetActiveNetworkObject(item->GetNetworkObject(), false);
            std::cout << "Is triggering" << std::endl;
        }
    });

    player->collisionListener->Attach(observer);
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

    SetActiveNetworkObject(players[freeIndex]->GetNetworkObject(), true);
    AttachCoinListener(players[freeIndex]);
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

void ServerGame::AddPlayerObjects(const Vector3 &position) {
    for (int i = 0; i < players.size(); i++) {
        float meshSize		= 1.0f;
        float inverseMass	= 0.5f;

        auto character = new GameObject();
        auto volume  = new AABBVolume(Vector3(meshSize * 0.5, meshSize * 0.5, meshSize * 0.5));

        character->SetBoundingVolume((CollisionVolume*)volume);

        character->GetTransform()
                .SetScale(Vector3(meshSize, meshSize, meshSize))
                .SetPosition(position);

        character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

        character->GetPhysicsObject()->SetInverseMass(inverseMass);
        character->GetPhysicsObject()->InitSphereInertia();

        character->SetNetworkObject(new NetworkObject(*character, i));
        netObjects.push_back(character->GetNetworkObject());

        character->collisionListener = new Subject<GameObject*>();

        character->name = "Player " + std::to_string(i);

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


