#pragma once
#include "NetworkBase.h"
#include "GameWorld.h"
#include <stdint.h>
#include <thread>
#include <atomic>

namespace NCL {
	namespace CSC8503 {
		class GameObject;
		class GameClient : public NetworkBase {
		public:
			GameClient();
			~GameClient();

			bool Connect(uint8_t a, uint8_t b, uint8_t c, uint8_t d, int portNum);
            void Disconnect();

            void SendPacket(GamePacket& payload);
            void SendPacket(GamePacket& payload, int something);

			void UpdateClient();

            int lastServerSnapshot;
            std::function<void()> connectCallback;
		protected:	
			_ENetPeer*	netPeer;
		};
	}
}

