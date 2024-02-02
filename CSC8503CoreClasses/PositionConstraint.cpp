#include "PositionConstraint.h"
//#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "PhysicsObject.h"
//#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

// Let me put a vector in this you stupid header file >:(
FixedConstraint::FixedConstraint(GameObject* attach, GameObject* attachTo, float a, float b, float c) {
    objectA		= attach;
    objectB		= attachTo;
    this->a = a;
    this->b = b;
    this->c = c;
}

void FixedConstraint::UpdateConstraint(float dt) {
    objectA->GetPhysicsObject()->SetLinearVelocity(Vector3());
    auto adjustedOffset = (Matrix4(objectB->GetTransform().GetOrientation())
            * Matrix4::Translation({a,b,c})).GetPositionVector();
    objectA->GetTransform().SetPosition(objectB->GetTransform().GetPosition() + adjustedOffset);
    objectA->GetTransform().SetOrientation(objectB->GetTransform().GetOrientation());
}

FixedConstraint::~FixedConstraint() {

}

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d)
{
	objectA		= a;
	objectB		= b;
	distance	= d;
}

PositionConstraint::~PositionConstraint()
{

}

void PositionConstraint::UpdateConstraint(float dt)	{
    Vector3 relativePos = objectA->GetTransform().GetPosition()
            - objectB->GetTransform().GetPosition();
    float currentDistance = relativePos.Length();
    float offset = distance - currentDistance;

    if (abs(offset) > 0.0f) {
        Vector3 offsetDir = relativePos.Normalised();

        PhysicsObject* physA = objectA->GetPhysicsObject();
        PhysicsObject* physB = objectB->GetPhysicsObject();

        Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();

        float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

        if (constraintMass > 0.0f) {
            float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);
            float biasFactor = 0.01f;
            float bias = -(biasFactor / dt) * offset;

            float lambda = -(velocityDot + bias) / constraintMass;

            Vector3 aImpulse = offsetDir * lambda;
            Vector3 bImpulse = -offsetDir * lambda;

            physA->ApplyLinearImpulse(aImpulse);
            physB->ApplyLinearImpulse(aImpulse);
        }
    }
}
