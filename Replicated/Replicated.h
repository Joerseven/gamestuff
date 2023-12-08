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

struct ServerMessagePacket : public GamePacket {
    short messageID;
    ServerMessagePacket() {
        type = Server_Message;
        size = sizeof(short);
    }
};

struct RemoteFunctionPacket : public GamePacket {
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
    SenderAcknowledger(R* sender, int id = -1): sentCounter(0) {
        thisSender = sender;
        targetId = id;
    }

    template <typename T>
    T& RequireAcknowledgement(const T& packet) {
        auto copy = packet;
        packets.emplace_back(++sentCounter, std::make_unique<GamePacket>((GamePacket&)packet));
        packet.acknowledge = sentCounter;
        return packet;
    }

    inline void ReceiveAcknowledgement(int playerStatus) {
        packets.erase(std::remove_if(packets.begin(), packets.end(), [&](std::pair<int, std::unique_ptr<GamePacket>>& p) {
            return p.first <= playerStatus;
        }), packets.end());
    }

    void CatchupPackets() {
        for (const auto& p : packets) {
            thisSender->SendPacket(p, targetId);
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
    RecieverAcknowledger(R* rm, int id = -1) : latestRecieved(0) {
        receiver = rm;
        targetId = id;
    }

    inline bool CheckAndUpdateAcknowledged(const GamePacket& packet) {
        if (latestRecieved == packet.acknowledge + 1) {
            latestRecieved++;
            return true;
        }

        return false;
    }

    void SendAcknowledgement() {
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
    Player_Jump
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
