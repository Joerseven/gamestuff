//
// Created by c3042750 on 01/12/2023.
//

#include "ClientGame.h"
#include "Window.h"

ClientGame::ClientGame() : controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()) {
    world = new GameWorld();
    renderer = new GameTechRenderer(*world);

    world->GetMainCamera().SetController(controller);

    controller.MapAxis(0, "Sidestep");
    controller.MapAxis(1, "UpDown");
    controller.MapAxis(2, "Forward");

    controller.MapAxis(3, "XLook");
    controller.MapAxis(4, "YLook");

    NetworkBase::Initialise();

    std::cout << "Client started successfully" << std::endl;

    InitialiseAssets();
}

ClientGame::~ClientGame() {
    delete world;
    delete renderer;
}

void ClientGame::UpdateGame(float dt) {
    UpdateKeys();

    world->GetMainCamera().UpdateCamera(dt);

    //Debug::DrawLine(Vector3(500, 0, 500), Vector3(500, 100, 500), Vector4(1, 0, 0, 1));

    world->UpdateWorld(dt);
    renderer->Update(dt);

    renderer->Render();
    //Debug::UpdateRenderables(dt);
}

void ClientGame::StartAsClient(char a, char b, char c, char d) {
    thisClient = new GameClient();
    thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

    thisClient->RegisterPacketHandler(Delta_State, this);
    thisClient->RegisterPacketHandler(Full_State, this);
    thisClient->RegisterPacketHandler(Player_Connected, this);
    thisClient->RegisterPacketHandler(Player_Disconnected, this);
    thisClient->RegisterPacketHandler(Message, this);

    std::cout << "Client started successfully" << std::endl;
}

void ClientGame::InitialiseAssets() {
    cubeMesh	= renderer->LoadMesh("cube.msh");
    sphereMesh	= renderer->LoadMesh("sphere.msh");
    charMesh	= renderer->LoadMesh("goat.msh");

    basicTex	= renderer->LoadTexture("checkerboard.png");
    basicShader = renderer->LoadShader("scene.vert", "scene.frag");

    InitCamera();
    InitWorld();
}

void ClientGame::InitCamera() {
    world->GetMainCamera().SetNearPlane(0.1f);
    world->GetMainCamera().SetFarPlane(500.0f);
    world->GetMainCamera().SetPitch(-15.0f);
    world->GetMainCamera().SetYaw(315.0f);
    world->GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}

void ClientGame::UpdateKeys() {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
        InitWorld(); //We can reset the simulation at any time with F1
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
        InitCamera(); //F2 will reset the camera to a specific default place
    }

}

void ClientGame::InitWorld() {
    world->ClearAndErase();
    AddFloorToWorld({0, -20, 0});
}

GameObject* ClientGame::AddFloorToWorld(const Vector3& position) {
    GameObject* floor = new GameObject();

    Vector3 floorSize = Vector3(200, 2, 200);
    AABBVolume* volume = new AABBVolume(floorSize);
    floor->SetBoundingVolume((CollisionVolume*)volume);
    floor->GetTransform()
            .SetScale(floorSize * 2)
            .SetPosition(position);

    floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));


    world->AddGameObject(floor);

    return floor;
}

void ClientGame::ReceivePacket(int type, GamePacket *payload, int source) {

}

int main() {
    Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1920, 1080);

    if (!w->HasInitialised()) {
        return -1;
    }

    w->ShowOSPointer(false);
    w->LockMouseToWindow(true);

    auto g = new ClientGame();
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

        w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

        g->UpdateGame(dt);
    }
    Window::DestroyGameWindow();
}
