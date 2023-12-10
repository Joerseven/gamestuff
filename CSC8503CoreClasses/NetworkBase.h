#pragma once
//#include "./enet/enet.h"
#include "lua.hpp"
#include "stb/stb_image.h"
struct _ENetHost;
struct _ENetPeer;
struct _ENetEvent;

enum BasicNetworkMessages {
	None,
	Message,
	String_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Player_Connected,
	Player_Disconnected,
	Shutdown,
    Server_Message,
    Acknowledge_Packet,
    Function // KEEP THIS LAST
};

struct GamePacket {
	short size;
	short type;
    int acknowledge;

	GamePacket() {
		type		= BasicNetworkMessages::None;
		size		= 0;
        acknowledge = -1;
	}

    virtual ~GamePacket() = default;

	GamePacket(short type) : GamePacket() {
		this->type	= type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct MessagePacket : public GamePacket {
    short playerID;
    short messageID;

    MessagePacket() {
        type = Message;
        size = sizeof(short) * 2;
    }
};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase	{
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 26656;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}
protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first	= range.first;
		last	= range.second;
		return true;
	}

	_ENetHost* netHandle;

	std::multimap<int, PacketReceiver*> packetHandlers;
};

struct StringPacket : public GamePacket {
    char stringData[256];

    StringPacket(const std::string& message) {
        type = BasicNetworkMessages::String_Message;
        size = (short)message.length();

        memcpy(stringData, message.data(), size);
    };

    std::string GetStringFromData() {
        std::string realString(stringData);
        realString.resize(size);
        return realString;
    }
};