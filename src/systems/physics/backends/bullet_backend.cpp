#include <systems/physics/backends/bullet_backend.h>
#include <core/utils/logger.h>

bool BulletBackend::Init()
{
    m_CollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_Dispatcher = std::make_unique<btCollisionDispatcher>(m_CollisionConfig.get());
    m_Broadphase = std::make_unique<btDbvtBroadphase>();
    m_Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_World = std::make_unique<btDiscreteDynamicsWorld>(
        m_Dispatcher.get(), m_Broadphase.get(), m_Solver.get(), m_CollisionConfig.get());

    m_World->setGravity(btVector3(0, -9.81f, 0));

    LOGGER_INFO("BulletBackend") << "Bullet physics initialized";
    return true;
}

void BulletBackend::Shutdown()
{
    m_World.reset();
    m_Solver.reset();
    m_Broadphase.reset();
    m_Dispatcher.reset();
    m_CollisionConfig.reset();
}

void BulletBackend::StepSimulation(float dt, int maxSubSteps)
{
    if (m_World)
        m_World->stepSimulation(dt, maxSubSteps);
}

void* BulletBackend::GetWorld()
{
    return m_World.get();
}
