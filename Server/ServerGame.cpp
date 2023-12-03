//
// Created by c3042750 on 01/12/2023.
//

#include "ServerGame.h"

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
    netIdCounter = 0;

    physics->UseGravity(true);

    ClearPlayers();

    InitWorld();

}

ServerGame::~ServerGame() {
    delete physics;
    delete world;
    delete server;
}

void ServerGame::UpdateGame(float dt) {

    server->UpdateServer();

    timeToNextPacket -= dt;
    if (timeToNextPacket < 0) {
        BroadcastSnapshot();
        timeToNextPacket += 1.0f / 20.0f;
    }

    world->UpdateWorld(dt);
    physics->Update(dt);


}

void ServerGame::BroadcastSnapshot() {
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

void ServerGame::InitWorld() {
    AddFloorToWorld({0, -20, 0});
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

    playerMap.insert(std::make_pair(peerId, freeIndex));
    players[playersJoined]->SetActive(true);

    std::cout << "Player added successfully!" << std::endl;

    return players[playersJoined];
}

void ServerGame::PlayerLeft(int peerId) {
    playersJoined--;
    players[playerMap[peerId]]->SetActive(false);
    playerMap.erase(peerId);

    std::cout << "Player removed successfully" << std::endl;

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


