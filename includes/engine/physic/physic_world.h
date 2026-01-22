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
    
    // 0: Fast, 1: Balanced, 2: Accurate
    void SetMode(int mode);

    void AddConstraint(btTypedConstraint* constraint, bool disableCollisionsBetweenLinkedBodies = true);
    void RemoveConstraint(btTypedConstraint* constraint);
    
    DebugDrawer* GetDebugDrawer() { return debugDrawer.get(); }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    std::unique_ptr<DebugDrawer> debugDrawer;

    std::vector<btCollisionShape *> m_collisionShapes;
    
    float m_linearSleepingThreshold = 0.8f;
    float m_angularSleepingThreshold = 1.0f;

    // Simulation settings
    float m_timeStep = 1.0f / 60.0f;
    int m_maxSubSteps = 1;
};