#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "Observer.h"
#include "StateMachine.h"

using std::vector;

namespace NCL::CSC8503 {
	class NetworkObject;
	class RenderObject;
	class PhysicsObject;

	class GameObject	{
	public:
		GameObject(const std::string& name = "");
		~GameObject();

		void SetBoundingVolume(CollisionVolume* vol) {
			boundingVolume = vol;
		}

		const CollisionVolume* GetBoundingVolume() const {
			return boundingVolume;
		}

		bool IsActive() const {
			return isActive;
		}

        inline void SetActive(bool active) {
            isActive = active;
        }

		Transform& GetTransform() {
			return transform;
		}

		RenderObject* GetRenderObject() const {
			return renderObject;
		}

		PhysicsObject* GetPhysicsObject() const {
			return physicsObject;
		}

		NetworkObject* GetNetworkObject() const {
			return networkObject;
		}

		void SetRenderObject(RenderObject* newObject) {
			renderObject = newObject;
		}

		void SetPhysicsObject(PhysicsObject* newObject) {
			physicsObject = newObject;
		}

        void SetNetworkObject(NetworkObject* newObject) {
            networkObject = newObject;
        }

		const std::string& GetName() const {
			return name;
		}

		void OnCollisionBegin(GameObject* otherObject) {
            if (collisionListener) {
                collisionListener->Trigger(otherObject);
            }
		}

		void OnCollisionEnd(GameObject* otherObject) {
		}

		bool GetBroadphaseAABB(Vector3&outsize) const;

		void UpdateBroadphaseAABB();

		void SetWorldID(int newID) {
			worldID = newID;
		}

		int		GetWorldID() const {
			return worldID;
		}

        Subject<GameObject*>* collisionListener;
        std::string	name;
        StateMachine*       stateMachine;
    protected:
		Transform			transform;

		CollisionVolume*	boundingVolume;
		PhysicsObject*		physicsObject;
		RenderObject*		renderObject;
		NetworkObject*		networkObject;



		bool		isActive;
		int			worldID;

        Vector3 broadphaseAABB;
	};
}

using namespace NCL::CSC8503;

