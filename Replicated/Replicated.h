//
// Created by c3042750 on 01/12/2023.
//

#ifndef CSC8503_REPLICATED_H
#define CSC8503_REPLICATED_H

#include "GameServer.h"

using namespace NCL;
using namespace CSC8503;

const float SERVERHERTZ = 1.0f / 20.0f;

struct ServerInfo {
    int playerIds[4];
};

struct PlayerScores {
    int values[4];
};

struct ServerMessagePacket : public GamePacket {
    short messageID;
    ServerMessagePacket() {
        type = Server_Message;
        size = sizeof(short);
    }
};

namespace Functions {
    enum Functions {
        AssignPlayerFunction = Function,
        SetNetworkObjectActive,
        UpdatePlayerScore
    };
}

struct AssignPlayerFunction {
    int networkId;
    int peerId;
};

struct SetNetworkObjectActive {
    int networkId;
    bool isActive;
};

#pragma pack(push, 1)
template <typename T>
struct FunctionPacket : public GamePacket {
    T info;
    FunctionPacket(T i, int function) {
        type = function;
        info = i;
        size = sizeof(FunctionPacket<T>) - sizeof(GamePacket);
    }
};
#pragma pack(pop)

enum Args {
    integer
};

// Get me the size of the variadic args

template <typename T, typename ...ta>
class SizeOf {
public:
    static const unsigned int size = sizeof(T) + SizeOf<ta...>::size;
};

template <typename T>
class SizeOf<T> {
public:
    static const unsigned int size = sizeof(T);
};


struct RemoteFunctionPacket : public GamePacket {
    std::string functionName;
    char parameters[];
    template <typename... Args>
    RemoteFunctionPacket(const std::string& name, const Args&... args) {
        functionName = name;
        int parameterSize = 0;
        int i = 0;
        ([&] {
            i++;
            std::cout << "Arg = " << args << ", size = " << sizeof(args) << std::endl;
            memcpy((void*)(parameters + parameterSize), &args, sizeof(args));
            parameterSize += sizeof(args);
        } (), ...);
    }
};

class RemoteFunctionManager {

    RemoteFunctionManager() {
    };

    void RegisterFunction(const char* name, std::vector<int>&& arguments) {
        boundFunctions.insert(std::make_pair(name, arguments));
    };

    void Receive() {

    };


    void SeperateArguments(char* data) {

    }

    void PushArguments() {

    }

    template <typename T, typename ...Args>
    void BindLocalFunction(T func, const Args&... args) {

    };

    std::unordered_map<std::string, std::vector<int>> boundFunctions;
private:
    lua_State* L;
};

struct AcknowledgePacket : public GamePacket {
    int acknowledge;
    AcknowledgePacket() {
        type = Acknowledge_Packet;
        size = sizeof(int);
    }
};

template <typename R>
class SenderAcknowledger {
public:
    SenderAcknowledger(R* sender, int id = -999): sentCounter(0) {
        thisSender = sender;
        targetId = id;
    }

    template <typename T>
    T& RequireAcknowledgement(T& packet) {
        packet.acknowledge = sentCounter;
        auto copy = packet;
        std::unique_ptr<GamePacket> p = std::make_unique<T>(copy);
        packets.emplace_back(sentCounter++, std::move(p));
        return packet;
    }

    inline void ReceiveAcknowledgement(int playerStatus) {
        packets.erase(std::remove_if(packets.begin(), packets.end(), [&](std::pair<int, std::unique_ptr<GamePacket>>& p) {
            return p.first <= playerStatus;
        }), packets.end());
    }

    void CatchupPackets() {
        for (const auto& p : packets) {
            //if (targetId != -999) {
                thisSender->SendPacket(*(p.second), targetId);
                std::cout << "Catch up packets: " << packets.size() << std::endl;
            //}
        }
    }

private:
    int sentCounter;
    int targetId;
    R* thisSender;
    std::vector<std::pair<int, std::unique_ptr<GamePacket>>> packets;
};

template <typename R>
class RecieverAcknowledger {
public:
    RecieverAcknowledger(R* rm, int id = -1) : latestRecieved(-1) {
        receiver = rm;
        targetId = id;
    }

    inline bool CheckAndUpdateAcknowledged(const GamePacket& packet) {
        // These don't need to be checked.
        if (packet.acknowledge == -1) {
            return true;
        }

        // This checks if next in acknowledge queue
        if (latestRecieved + 1 == packet.acknowledge) {
            latestRecieved++;
            std::cout << "Packet recieved: " << packet.acknowledge << ", " << "Last Packet: " << latestRecieved << std::endl;
            return true;
        }

        return false;
    }

    void SendAcknowledgement() {
        int t = -2;
        AcknowledgePacket p;
        p.acknowledge = latestRecieved;
        receiver->SendPacket(p, targetId);
    }
private:
    R* receiver;
    int targetId;
    int latestRecieved;
};

// Add different messages here
enum ServerMessages {
    Player_Loaded,
    Player_Jump,
    Player_Created,
    Player_Use,
    Player_Ready,
};

enum PlayerMessages {
    Game_Started
};


enum ObjectTypes {
    Block,
};



// Questions
/*
 * 1. handling int overflow on the state ids?
 * 2. better organisation with gameobjects and client/server?
 * 3.
 */

#endif //CSC8503_REPLICATED_H
