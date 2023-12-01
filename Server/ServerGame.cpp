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

    InitWorld();

}

ServerGame::~ServerGame() {
    delete physics;
    delete world;
    delete server;
}

void ServerGame::UpdateGame(float dt) {
    timeToNextPacket -= dt;
    if (timeToNextPacket < 0) {
        BroadcastSnapshot();
        timeToNextPacket += 1.0f / 20.0f;
    }
    server->UpdateServer();
}

void ServerGame::BroadcastSnapshot() {
    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;

    world->GetObjectIterators(first, last);

    for (auto i = first; i != last; ++i) {
        NetworkObject* o = (*i)->GetNetworkObject();
        if (!o) {
            continue;
        }

        GamePacket* newPacket = nullptr;
        if (o->WritePacket(&newPacket, false, ++currentSnapshot)) {
            server->SendGlobalPacket(*newPacket);
            delete newPacket;
        }
    }
}

void ServerGame::InitWorld() {

}

void DebugPackets(int type, GamePacket *payload, int source) {
    std::cout << "Recieved Packet: " << std::endl;
    std::cout << "type: " << type << std::endl;
    std::cout << "source: " << source << std::endl;
}
void ServerGame::ReceivePacket(int type, GamePacket *payload, int source) {
    DebugPackets(type, payload, source);
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

int main() {

    auto timer = GameTimer();

    auto s = new ServerGame();
    timer.GetTimeDeltaSeconds();

    while (true) {
        timer.Tick();
        float dt = timer.GetTimeDeltaSeconds();

        if (dt > 0.1f) {
            std::cout << "Skipping large time delta" << std::endl;
            continue; //must have hit a breakpoint or something to have a 1 second frame time!
        }

        s->UpdateGame(dt);
    }

}


