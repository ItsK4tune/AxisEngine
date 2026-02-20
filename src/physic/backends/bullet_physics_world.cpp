#include <physic/backends/bullet_physics_world.h>
#include <physic/backends/bullet_debug_drawer.h>
#include <utils/logger.h>

BulletPhysicsWorld::BulletPhysicsWorld()
{
}

BulletPhysicsWorld::~BulletPhysicsWorld()
{
    Clear();
}

void BulletPhysicsWorld::Init()
{
    m_CollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_Dispatcher = std::make_unique<btCollisionDispatcher>(m_CollisionConfig.get());
    m_OverlappingPairCache = std::make_unique<btDbvtBroadphase>();
    m_Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_DynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_Dispatcher.get(), m_OverlappingPairCache.get(), m_Solver.get(), m_CollisionConfig.get());

    m_DynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

    m_OwnedDebugDrawer = std::make_unique<BulletDebugDrawer>();
    m_OwnedDebugDrawer->Init();
    m_OwnedDebugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    m_CurrentDebugDrawer = m_OwnedDebugDrawer.get();
    m_DynamicsWorld->setDebugDrawer(m_CurrentDebugDrawer);
}

void BulletPhysicsWorld::Update(float dt)
{
    if (m_DynamicsWorld)
    {
        m_DynamicsWorld->stepSimulation(dt, 10, 1.0f / 60.0f);
    }
}

void BulletPhysicsWorld::Clear()
{
    if (m_DynamicsWorld)
    {
        for (int i = m_DynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
        {
            btCollisionObject* obj = m_DynamicsWorld->getCollisionObjectArray()[i];
            m_DynamicsWorld->removeCollisionObject(obj);
        }
    }
    m_OwnedDebugDrawer.reset();
    m_CurrentDebugDrawer = nullptr;
}

void BulletPhysicsWorld::SetGravity(const glm::vec3& gravity)
{
    if (m_DynamicsWorld)
        m_DynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

void BulletPhysicsWorld::SetMode(int mode)
{
}

void BulletPhysicsWorld::AddRigidBody(IRigidBody* body)
{
    if (!m_DynamicsWorld || !body) return;

    BulletRigidBody* bBody = static_cast<BulletRigidBody*>(body);
    if (bBody->GetRaw())
    {
        m_DynamicsWorld->addRigidBody(bBody->GetRaw());
    }
}

void BulletPhysicsWorld::RemoveRigidBody(IRigidBody* body)
{
    if (!m_DynamicsWorld || !body) return;

    BulletRigidBody* bBody = static_cast<BulletRigidBody*>(body);
    if (bBody->GetRaw())
    {
        m_DynamicsWorld->removeRigidBody(bBody->GetRaw());
    }
}

void BulletPhysicsWorld::DebugDraw()
{
    if (m_CurrentDebugDrawer)
        m_CurrentDebugDrawer->FrameStart();

    if (m_DynamicsWorld)
        m_DynamicsWorld->debugDrawWorld();

    if (m_CurrentDebugDrawer)
        m_CurrentDebugDrawer->Flush();
}

std::shared_ptr<IRigidBody> BulletPhysicsWorld::CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape)
{
    if (!shape) return nullptr;

    BulletCollisionShape* bShape = static_cast<BulletCollisionShape*>(shape.get());

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));
    startTransform.setRotation(btQuaternion(startRot.x, startRot.y, startRot.z, startRot.w));

    bool isDynamic = (mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic && bShape->GetRaw())
        bShape->GetRaw()->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, bShape->GetRaw(), localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);

    return std::make_shared<BulletRigidBody>(body, shape);
}

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateBoxShape(const glm::vec3& halfExtents)
{
    btBoxShape* shape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
    return std::make_shared<BulletCollisionShape>(shape, CollisionShapeType::Box);
}

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateSphereShape(float radius)
{
    btSphereShape* shape = new btSphereShape(radius);
    return std::make_shared<BulletCollisionShape>(shape, CollisionShapeType::Sphere);
}

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateCapsuleShape(float radius, float height)
{
    btCapsuleShape* shape = new btCapsuleShape(radius, height);
    return std::make_shared<BulletCollisionShape>(shape, CollisionShapeType::Capsule);
}

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateCompoundShape()
{
    btCompoundShape* shape = new btCompoundShape();
    return std::make_shared<BulletCollisionShape>(shape, CollisionShapeType::CompoundHull);
}

void BulletPhysicsWorld::AddChildShape(ICollisionShape* parent, ICollisionShape* child, const glm::vec3& pos, const glm::quat& rot)
{
    if (!parent || !child) return;

    BulletCollisionShape* bParent = static_cast<BulletCollisionShape*>(parent);
    BulletCollisionShape* bChild = static_cast<BulletCollisionShape*>(child);

    if (bParent->GetRaw() && bChild->GetRaw() && bParent->GetRaw()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
    {
        btCompoundShape* compound = static_cast<btCompoundShape*>(bParent->GetRaw());

        btTransform localTrans;
        localTrans.setIdentity();
        localTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
        localTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

        compound->addChildShape(localTrans, bChild->GetRaw());
    }
}
