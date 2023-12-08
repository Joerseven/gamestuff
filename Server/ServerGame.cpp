//
// Created by c3042750 on 01/12/2023.
//

#include "ServerGame.h"

GameObject* moreTheFloor;

ServerGame::ServerGame() {

    NetworkBase::Initialise();
    server = new GameServer(NetworkBase::GetDefaultPort(), 4);
    server->RegisterPacketHandler(Received_State, this);
    server->RegisterPacketHandler(Server_Message, this);

    std::cout << "Server starting up" << std::endl;

    world = new GameWorld();
    physics = new PhysicsSystem(*world);

    forceMagnitude = 10.0f;
    timeToNextPacket  = 0.0f;
    netIdCounter = 4; // Players added will bring it up to 4.

    physics->UseGravity(true);

    ClearPlayers();

    AddPlayerObjects(Vector3(0,60,0));

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    RegisterFunctions(L);

    auto status = luaL_dofile(L, ASSETROOTLOCATION "Data/Levels.lua");

    if (status) {
        std::cerr << "Lua file giga dead: " << lua_tostring(L, -1);
        exit(1);
    }

    LoadLevel(L, 1);

    lua_close(L);

}

ServerGame::~ServerGame() {
    delete physics;
    delete world;
    delete server;
    std::cout << "Sent Packets: " <<  sentPackets << std::endl;
}

void ServerGame::UpdateGame(float dt) {

    UpdatePlayers();

    timeToNextPacket -= dt;
    if (timeToNextPacket < 0) {
        BroadcastSnapshot();
        timeToNextPacket += SERVERHERTZ;
    }

    world->UpdateWorld(dt);
    physics->Update(dt);

    //std::cout << "Player position: " << players[0]->GetTransform().GetPosition();

    server->UpdateServer();
}

void ServerGame::UpdatePlayers() {
    for (auto it = playerControls.begin(); it != playerControls.end(); it++) {
        auto &pressed = it->second;
        float mag = 0.2f;
        players[playerMap[it->first]]->GetPhysicsObject()->AddForce(Vector3(
                (float)pressed[3] * mag + (float)pressed[1] * mag * -1,
                0,
                (float)pressed[2] * mag + (float)pressed[0] * mag * -1));
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
    //DebugPackets(type, payload, source);
    if (type == Server_Message) {
        auto id = ((ServerMessagePacket*)payload)->messageID;
        if (id == Player_Loaded) {
            CreatePlayer(source);
        } else if (id == Player_Jump) {
            std::cout << "Big wins" << std::endl;
            players[playerMap[source]]->GetPhysicsObject()->AddForce(Vector3(0, 1000, 0));
        }
    }
    if (type == Received_State) {
        auto exists = playerMap.find(source) != playerMap.end();
        if (exists) {
            auto pressed = ((ClientPacket*)payload)->buttonstates;
            float mag = 100.0f;
            memcpy(playerControls[source].data(), pressed, 8*sizeof(char));
        }
    }
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

    playerSenders.insert(std::make_pair(peerId, new SenderAcknowledger<GameServer>(server, peerId)));
    playerMap.insert(std::make_pair(peerId, freeIndex));
    players[playersJoined]->SetActive(true);

    std::cout << "Player added successfully!" << std::endl;

    serverInfo.playerIds[freeIndex] = peerId;

    return players[playersJoined];
}

void ServerGame::PlayerLeft(int peerId) {

    players[playerMap[peerId]]->SetActive(false);
    serverInfo.playerIds[playerMap[peerId]] = -1;

    playerMap.erase(peerId);

    playersJoined--;


    std::cout << "Player removed successfully" << std::endl;

}

void ServerGame::LoadLevel(lua_State *L, int level) {
    lua_getglobal(L, "levels");
    lua_pushnumber(L, level);
    lua_gettable(L, -2);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        std::cout << lua_typename(L, lua_type(L, -2)) << lua_typename(L, lua_type(L, -1)) << std::endl;
        AddObjectFromLua(L);
        lua_pop(L, 1);
    }
}

void ServerGame::AddObjectFromLua(lua_State *L) {
    // object table is on top of the stack
    GameObject* g = new GameObject();
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
        auto volume  = new SphereVolume(1.0f);

        character->SetBoundingVolume((CollisionVolume*)volume);

        character->GetTransform()
                .SetScale(Vector3(meshSize, meshSize, meshSize))
                .SetPosition(position);

        character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

        character->GetPhysicsObject()->SetInverseMass(inverseMass);
        character->GetPhysicsObject()->InitSphereInertia();

        character->SetNetworkObject(new NetworkObject(*character, i));

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


