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

    netIdCounter = 4;



    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    RegisterFunctions(L);

    auto status = luaL_dofile(L, ASSETROOTLOCATION "Data/Levels.lua");

    if (status) {
        std::cerr << "Lua file giga dead: " << lua_tostring(L, -1);
        exit(1);
    }

    InitCamera();
    InitWorld();

    lua_getglobal(L, "spawnPoint");
    auto spawnPoint = Vector3((float)getNumberField(L, "x"), (float)getNumberField(L, "y"), (float)getNumberField(L, "z"));
    lua_pop(L, 1);

    AddPlayerObjects(spawnPoint);

    LoadLevel(L, 1);

    lua_close(L);

    tweenManager = new TweenManager();

    StartAsClient(127, 0, 0, 1);

    recieverAcknowledger = new RecieverAcknowledger(thisClient);
    senderAcknowledger = new SenderAcknowledger(thisClient);



    //RemoteFunctionPacket(2, "Hello world", 27, 4921231, 18);
}

ClientGame::~ClientGame() {
    delete world;
    delete renderer;
    delete tweenManager;

    std::cout << "Recieved Packets" << recievedPackets << std::endl;
}

void ClientGame::UpdateGame(float dt) {
    GetClientInput();

    world->GetMainCamera().UpdateCamera(dt);

    //Debug::DrawLine(Vector3(500, 0, 500), Vector3(500, 100, 500), Vector4(1, 0, 0, 1));

    world->UpdateWorld(dt);

    tweenManager->Update(dt);

    renderer->Update(dt);

    recieverAcknowledger->SendAcknowledgement();
    senderAcknowledger->CatchupPackets();

    renderer->Render();
    thisClient->UpdateClient();
    //Debug::UpdateRenderables(dt);
}

void ClientGame::LoadLevel(lua_State *L, int level) {
    lua_getglobal(L, "levels");
    lua_pushnumber(L, level);
    lua_gettable(L, -2);

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        AddObjectFromLua(L);
        lua_pop(L, 1);
    }
}

void ClientGame::AddObjectFromLua(lua_State *L) {
    // object table is on top of the stack
    GameObject* g = new GameObject();
    g->SetActive(getBool(L, "active"));
    Vector3 size = getVec3Field(L, "size");
    Vector3 position = getVec3Field(L, "position");

    g->GetTransform()
            .SetPosition(position)
            .SetScale(size);

    auto volume = getStringField(L, "bounding");
    AddVolume(g, std::string(volume), L);

    if (getBool(L, "network")) {
        g->SetNetworkObject(new NetworkObject(*g, ++netIdCounter));
    }

    auto t = getStringField(L, "texture");

    g->SetRenderObject(new RenderObject(&g->GetTransform(),
                                        GetMesh(getStringField(L, "mesh")),
                                        std::string(t) == std::string("none") ? nullptr : GetTexture(t),
                                        GetShader(getStringField(L, "shader"))));



    //Debug::DrawBoundingVolume(&g->GetTransform(), const_cast<CollisionVolume *>(g->GetBoundingVolume()), Vector4(1.0, 0.5, 0.2, 1.0));

    world->AddGameObject(g);
}

void ClientGame::AddVolume(GameObject* g, const std::string& volumeType, lua_State *L) {
    if ("SphereVolume" == volumeType) {
        auto size = getNumberField(L, "boundingSize");
        auto volume  = new SphereVolume(size);
        g->SetBoundingVolume((CollisionVolume*)volume);
    }

    if ("AABBVolume" == volumeType) {
        auto size = getVec3Field(L, "boundingSize");
        auto volume = new AABBVolume(size);
        g->SetBoundingVolume((CollisionVolume*)volume);
    }
}

void ClientGame::StartAsClient(char a, char b, char c, char d) {
    thisClient = new GameClient();
    thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

    thisClient->RegisterPacketHandler(Delta_State, this);
    thisClient->RegisterPacketHandler(Full_State, this);
    thisClient->RegisterPacketHandler(Player_Connected, this);
    thisClient->RegisterPacketHandler(Player_Disconnected, this);
    thisClient->RegisterPacketHandler(Message, this);
    thisClient->RegisterPacketHandler(String_Message, this);
    thisClient->RegisterPacketHandler(Acknowledge_Packet, this);
    thisClient->RegisterPacketHandler(Assign_Player, this);

    thisClient->connectCallback = [&](){
        ServerMessagePacket p;
        p.messageID = Player_Loaded;
        thisClient->SendPacket(p);
    };

    std::cout << "Client started successfully" << std::endl;
}

void ClientGame::InitialiseAssets() {


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
}


void ClientGame::ReceivePacket(int type, GamePacket *payload, int source) {

    if (!(recieverAcknowledger->CheckAndUpdateAcknowledged(*payload))) {
        return;
    }

    if (type == Full_State) {
        netObjects[((FullPacket*)payload)->objectID]->ReadPacket(*payload, tweenManager);
    }

    if (type == Message) {
        if (((MessagePacket*)payload)->messageID == Player_Created) {

        }
    }

    if (type == Acknowledge_Packet) {
        senderAcknowledger->ReceiveAcknowledgement(((AcknowledgePacket*)payload)->acknowledge);
    }

    if (type == Assign_Player) {
        std::cout << ((AssignPlayerPacket*)payload)->networkId << std::endl;
        netObjects[((AssignPlayerPacket*)payload)->networkId]->object.SetActive(true);
    }
}

void ClientGame::AddPlayerObjects(const Vector3 &position) {

    netObjects.resize(4);

    for (int i = 0; i < 4; i++) {
        float meshSize		= 1.0f;
        float inverseMass	= 0.5f;

        auto character = new GameObject();
        auto volume  = new SphereVolume(1.0f);

        character->SetBoundingVolume((CollisionVolume*)volume);

        character->GetTransform()
                .SetScale(Vector3(meshSize, meshSize, meshSize))
                .SetPosition(position);

        character->SetNetworkObject(new NetworkObject(*character, i));
        character->SetRenderObject(new RenderObject(&character->GetTransform(), GetMesh("Goat"), nullptr, GetShader("scene")));
        netObjects[i] = character->GetNetworkObject();
        character->SetActive(false);

        //Debug::DrawBoundingVolume(&character->GetTransform(), (CollisionVolume*)volume, Vector4(1.0, 0.5, 0.2, 1.0));

        world->AddGameObject(character);
    }
}

void ClientGame::UpdateCamera() {
    if (lockedObject != nullptr) {
        Vector3 objPos = lockedObject->GetTransform().GetPosition();
        Vector3 camPos = objPos + lockedOffset;

        Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

        Matrix4 modelMat = temp.Inverse();

        Quaternion q(modelMat);
        Vector3 angles = q.ToEuler(); //nearly there now!

        world->GetMainCamera().SetPosition(camPos);
        world->GetMainCamera().SetPitch(angles.x);
        world->GetMainCamera().SetYaw(angles.y);
    }
}

Texture *ClientGame::GetTexture(const std::string &texture) {
    if (textures.find(texture) == textures.end()) {
        auto p = renderer->LoadTexture(texture + std::string(".png"));
        textures.insert(std::make_pair(texture, p));
        return p;
    }

    return textures[texture];
}

Mesh *ClientGame::GetMesh(const std::string &mesh) {
    if (meshes.find(mesh) == meshes.end()) {
        auto p = renderer->LoadMesh(mesh + std::string(".msh"));
        meshes.insert(std::make_pair(mesh, p));
        return p;
    }

    return meshes[mesh];
}

Shader *ClientGame::GetShader(const std::string &shader) {
    if (shaders.find(shader) == shaders.end()) {
        auto p = renderer->LoadShader(shader + std::string(".vert"), shader + std::string(".frag"));
        shaders.insert(std::make_pair(shader, p));
        return p;
    }

    return shaders[shader];
}

void ClientGame::GetClientInput() {
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

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::J)) {
        ServerMessagePacket p;
        p.messageID = Player_Jump;
        thisClient->SendPacket(p);
    }

    thisClient->SendPacket(newPacket);
}

void ClientGame::Disconnect() {
    thisClient->Disconnect();
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
    g->Disconnect();
    Window::DestroyGameWindow();
}
