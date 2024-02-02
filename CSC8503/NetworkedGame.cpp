#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"

#define COLLISION_MSG 30


NetworkedGame::NetworkedGame()	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {

	//thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);

	StartLevel();

    std::cout << "Server started successfully" << std::endl;
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);

	SpawnPlayer();

    std::cout << "Client started successfully" << std::endl;
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (thisServer) {
			//UpdateAsServer(dt);
		}
		if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyCodes::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyCodes::F10)) {
		StartAsClient(127,0,0,1);
	}

    if (thisClient) {
        thisClient->UpdateClient();
    }

    if (thisServer) {
        thisServer->UpdateServer();
    }

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
}

void NetworkedGame::UpdateAsClient(float dt) {
    ClientPacket newPacket;

    for (size_t s = 0; s < 8; s++) {
        newPacket.buttonstates[s] = 0;
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
        newPacket.buttonstates[0] = 1;
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
        newPacket.buttonstates[1] = 1;
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
        newPacket.buttonstates[2] = 1;
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
        newPacket.buttonstates[3] = 1;
    }

    thisClient->SendPacket(newPacket);
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}

		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket, deltaFrame, ++thisServer->currentSnapshot)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
	}
}

void NetworkedGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = std::min(minID, i.second);
		maxID = std::max(maxID, i.second);
	}
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

void NetworkedGame::SpawnPlayer() {
    localPlayer = AddPlayerToWorld(Vector3(0,0,0));
}

void NetworkedGame::StartLevel() {
    InitWorld();
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	if (thisServer) {
        if (type == Received_State) {
            if (localPlayer) {
                auto pressed = ((ClientPacket*)payload)->buttonstates;
                float mag = 100.0f;
                localPlayer->GetPhysicsObject()->AddForce(Vector3(
                        (float)pressed[3] * mag + (float)pressed[1] * mag * -1,
                        0,
                        (float)pressed[2] * mag + (float)pressed[0] * mag * -1));
            }
        }
    }
}

void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID  = a->GetPlayerNum();

		thisClient->SendPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisClient->SendPacket(newPacket);
	}
}