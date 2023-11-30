#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

void TestStateMachine() {
    auto testMachine = new StateMachine();
    int data = 0;

    auto A = new State([&](float dt)->void {
        std::cout << "I'm state A!" << std::endl;
        data++;
    });

    auto B = new State([&](float dt)->void {
        std::cout << "I'm state B!" << std::endl;
        data--;
    });

    auto stateAB = new StateTransition(A, B, [&]()->bool {
        return data > 10;
    });

    auto stateBA = new StateTransition(B, A, [&]()->bool {
        return data < 0;
    });

    testMachine->AddState(A);
    testMachine->AddState(B);
    testMachine->AddTransition(stateAB);
    testMachine->AddTransition(stateBA);

    for (int i = 0; i < 100; i++) {
        testMachine->Update(1.0f);
    }
}

void TestPathfinding() {
}

void DisplayPathfinding() {
}

class TestPacketReceiver : public PacketReceiver {
public:
    explicit TestPacketReceiver(std::string n) {
        this->name = n;
    }

    void ReceivePacket(int type, GamePacket* payload, int source) override {
        if (type == String_Message) {
            StringPacket* realPacket = (StringPacket*)payload;

            std::string msg = realPacket->GetStringFromData();

            std::cout << name << " recieved message: " << msg << std::endl;
        }
    }
protected:
    std::string name;
};

void TestNetworking() {
    NetworkBase::Initialise();

    TestPacketReceiver serverReciever("Server");
    TestPacketReceiver clientReciever("Client");

    int port = NetworkBase::GetDefaultPort();

    GameServer* server = new GameServer(port, 1);
    GameClient* client = new GameClient();

    server->RegisterPacketHandler(String_Message, &serverReciever);
    client->RegisterPacketHandler(String_Message, &clientReciever);

    auto test1 = StringPacket("Server says hello! " + std::to_string(15));
    auto test2 = StringPacket("Client says hello! " + std::to_string(12));

    bool canConnect = client->Connect(127, 0, 0, 1, port);
    for (int i = 0; i < 100; i++) {
        server->SendGlobalPacket(test1);
        client->SendPacket(test2);

        server->UpdateServer();
        client->UpdateClient();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    NetworkBase::Destroy();
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
    TestNetworking();
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1920, 1080);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	auto g = new NetworkedGame();
	w->GetTimer().GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
		float dt = w->GetTimer().GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyCodes::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyCodes::T)) {
			w->SetWindowPosition(0, 0);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		g->UpdateGame(dt);
	}
	Window::DestroyGameWindow();
}