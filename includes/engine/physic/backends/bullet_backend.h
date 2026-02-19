#pragma once

#include <interface/physics/i_physics_backend.h>
#include <btBulletDynamicsCommon.h>
#include <memory>

class BulletBackend : public IPhysicsBackend
{
public:
    bool Init() override;
    void Shutdown() override;
    void StepSimulation(float dt, int maxSubSteps = 1) override;

    void* GetWorld() override;
    std::string GetName() const override { return "Bullet"; }

    btDiscreteDynamicsWorld* GetDynamicsWorld() { return m_World.get(); }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_CollisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_Dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_Broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_World;
};
