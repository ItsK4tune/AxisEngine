#include <engine/physic/physic_world.h>

PhysicsWorld::
    PhysicsWorld::PhysicsWorld()
{
    collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
    overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfig.get());
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

void PhysicsWorld::Update(float dt)
{
    dynamicsWorld->stepSimulation(dt, 10);
}

btDiscreteDynamicsWorld *PhysicsWorld::GetWorld() { return dynamicsWorld.get(); }

btRigidBody *PhysicsWorld::CreateRigidBody(float mass, const btTransform &startTransform, btCollisionShape *shape)
{
    bool isDynamic = (mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody *body = new btRigidBody(rbInfo);

    dynamicsWorld->addRigidBody(body);
    return body;
}