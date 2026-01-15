#pragma once

#include <btBulletDynamicsCommon.h>
#include <memory>
#include <vector>
#include <physic/debug_drawer.h>

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void Update(float dt);
    void Clear();

    btDiscreteDynamicsWorld *GetWorld();
    btRigidBody *CreateRigidBody(float mass, const btTransform &startTransform, btCollisionShape *shape);
    void RegisterShape(btCollisionShape* shape);
    
    DebugDrawer* GetDebugDrawer() { return debugDrawer.get(); }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    std::unique_ptr<DebugDrawer> debugDrawer;

    std::vector<btCollisionShape *> m_collisionShapes;
};