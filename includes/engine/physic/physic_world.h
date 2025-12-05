#pragma once
#include <btBulletDynamicsCommon.h>
#include <memory>

class PhysicsWorld {
public:
    PhysicsWorld() {
        collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
        dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
        overlappingPairCache = std::make_unique<btDbvtBroadphase>();
        solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
            dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfig.get()
        );
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    }

    void Update(float dt) {
        dynamicsWorld->stepSimulation(dt, 10);
    }

    btDiscreteDynamicsWorld* GetWorld() { return dynamicsWorld.get(); }

    btRigidBody* CreateRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape) {
        bool isDynamic = (mass != 0.f);
        btVector3 localInertia(0, 0, 0);
        if (isDynamic) shape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);

        dynamicsWorld->addRigidBody(body);
        return body;
    }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
};