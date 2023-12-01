//
// Created by c3042750 on 01/12/2023.
//

#ifndef CSC8503_REPLICATED_H
#define CSC8503_REPLICATED_H

struct ServerMessagePacket : public GamePacket {
    short messageID;
    ServerMessagePacket() {
        type = Server_Message;
        size = sizeof(short);
    }
};
enum ServerMessages {
    Player_Loaded
};

#endif //CSC8503_REPLICATED_H
