//
// Created by c3042750 on 01/12/2023.
//

#ifndef CSC8503_REPLICATED_H
#define CSC8503_REPLICATED_H

const float SERVERHERTZ = 1.0f / 20.0f;

struct ServerMessagePacket : public GamePacket {
    short messageID;
    ServerMessagePacket() {
        type = Server_Message;
        size = sizeof(short);
    }
};

// Add different messages here
enum ServerMessages {
    Player_Loaded,
    Player_Jump
};



// Questions
/*
 * 1. handling int overflow on the state ids?
 * 2. better organisation with gameobjects and client/server?
 * 3.
 */

#endif //CSC8503_REPLICATED_H
