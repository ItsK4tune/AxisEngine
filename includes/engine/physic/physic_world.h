#pragma once
#include <btBulletDynamicsCommon.h>
#include <memory>

class PhysicsWorld
{
public:
    PhysicsWorld();

    void Update(float dt);
    btDiscreteDynamicsWorld *GetWorld();
    btRigidBody *CreateRigidBody(float mass, const btTransform &startTransform, btCollisionShape *shape);

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
};