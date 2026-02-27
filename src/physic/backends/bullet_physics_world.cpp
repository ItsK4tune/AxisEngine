#include <physic/backends/bullet_physics_world.h>
#include <physic/backends/bullet_debug_drawer.h>
#include <physic/backends/bullet_constraint.h>
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
    m_Dispatcher = std::make_unique<CustomCollisionDispatcher>(m_CollisionConfig.get());
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
        for (int i = m_DynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
        {
            btTypedConstraint* constraint = m_DynamicsWorld->getConstraint(i);
            m_DynamicsWorld->removeConstraint(constraint);
        }

        for (int i = m_DynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
        {
            btCollisionObject* obj = m_DynamicsWorld->getCollisionObjectArray()[i];
            m_DynamicsWorld->removeCollisionObject(obj);
        }
    }
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

void BulletPhysicsWorld::SetCollisionFilter(CollisionFilterCallback callback)
{
    if (m_Dispatcher)
    {
        m_Dispatcher->SetFilterCallback(callback);
    }
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

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateMeshShape(const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
{
    if (vertices.empty() || indices.empty()) return nullptr;

    std::vector<float> vs = vertices;
    std::vector<uint32_t> is = indices;

    btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray(
        is.size() / 3,
        (int*)is.data(),
        3 * sizeof(uint32_t),
        vs.size() / 3,
        (float*)vs.data(),
        3 * sizeof(float)
    );

    bool useQuantizedAabbCompression = true;
    btBvhTriangleMeshShape* bvhShape = new btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression);

    return std::make_shared<BulletMeshCollisionShape>(bvhShape, meshInterface, std::move(vs), std::move(is));
}

void BulletPhysicsWorld::AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child, const glm::vec3& pos, const glm::quat& rot)
{
    if (!parent || !child) return;

    BulletCollisionShape* bParent = static_cast<BulletCollisionShape*>(parent.get());
    BulletCollisionShape* bChild = static_cast<BulletCollisionShape*>(child.get());

    if (bParent->GetRaw() && bChild->GetRaw() && bParent->GetRaw()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
    {
        btCompoundShape* compound = static_cast<btCompoundShape*>(bParent->GetRaw());

        btTransform localTrans;
        localTrans.setIdentity();
        localTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
        localTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

        compound->addChildShape(localTrans, bChild->GetRaw());
        parent->AddChild(child);
    }
}

void BulletPhysicsWorld::AddConstraint(std::shared_ptr<IConstraint> constraint)
{
    if (!m_DynamicsWorld || !constraint) return;
    BulletConstraint* bConst = static_cast<BulletConstraint*>(constraint.get());
    if (bConst->GetRaw())
    {
        m_DynamicsWorld->addConstraint(bConst->GetRaw());
    }
}

void BulletPhysicsWorld::RemoveConstraint(std::shared_ptr<IConstraint> constraint)
{
    if (!m_DynamicsWorld || !constraint) return;
    BulletConstraint* bConst = static_cast<BulletConstraint*>(constraint.get());
    if (bConst->GetRaw())
    {
        m_DynamicsWorld->removeConstraint(bConst->GetRaw());
    }
}

std::shared_ptr<IConstraint> BulletPhysicsWorld::CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB)
{
    if (!rbA || !rbB) return nullptr;
    BulletRigidBody* bA = static_cast<BulletRigidBody*>(rbA.get());
    BulletRigidBody* bB = static_cast<BulletRigidBody*>(rbB.get());
    if (!bA->GetRaw() || !bB->GetRaw()) return nullptr;

    btPoint2PointConstraint* p2p = new btPoint2PointConstraint(
        *bA->GetRaw(), 
        *bB->GetRaw(), 
        btVector3(pivotInA.x, pivotInA.y, pivotInA.z), 
        btVector3(pivotInB.x, pivotInB.y, pivotInB.z)
    );
    return std::make_shared<BulletConstraint>(p2p);
}

std::shared_ptr<IConstraint> BulletPhysicsWorld::CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::vec3& axisInA, const glm::vec3& axisInB)
{
    if (!rbA || !rbB) return nullptr;
    BulletRigidBody* bA = static_cast<BulletRigidBody*>(rbA.get());
    BulletRigidBody* bB = static_cast<BulletRigidBody*>(rbB.get());
    if (!bA->GetRaw() || !bB->GetRaw()) return nullptr;

    btHingeConstraint* hinge = new btHingeConstraint(
        *bA->GetRaw(),
        *bB->GetRaw(),
        btVector3(pivotInA.x, pivotInA.y, pivotInA.z),
        btVector3(pivotInB.x, pivotInB.y, pivotInB.z),
        btVector3(axisInA.x, axisInA.y, axisInA.z),
        btVector3(axisInB.x, axisInB.y, axisInB.z)
    );
    return std::make_shared<BulletConstraint>(hinge);
}
