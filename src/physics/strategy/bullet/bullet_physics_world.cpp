#include <physics/strategy/bullet/bullet_constraint.h>
#include <physics/strategy/bullet/bullet_debug_drawer.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <physics/strategy/bullet/bullet_character_controller.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <core/logic/logger.h>
#include <physics/interface/i_collision_shape.h>

// Custom callback to ignore a specific entity
struct IgnoreEntityRayCallback : public btCollisionWorld::ClosestRayResultCallback {
    entt::entity m_Ignore;
    IgnoreEntityRayCallback(const btVector3& start, const btVector3& end, entt::entity ignore)
        : btCollisionWorld::ClosestRayResultCallback(start, end), m_Ignore(ignore) {}

    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
        if (m_Ignore != entt::null && rayResult.m_collisionObject->getUserPointer()) {
            // Offset logic: getUserPointer returns entity + 1
            entt::entity hitEnt = (entt::entity)((uintptr_t)rayResult.m_collisionObject->getUserPointer() - 1);
            if (hitEnt == m_Ignore) return 1.0; 
        }
        return btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
};

BulletPhysicsWorld::BulletPhysicsWorld()
{
}

BulletPhysicsWorld::~BulletPhysicsWorld()
{
    Clear();
}

void BulletPhysicsWorld::Initialize()
{
    m_CollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_Dispatcher = std::make_unique<CustomCollisionDispatcher>(m_CollisionConfig.get());
    m_OverlappingPairCache = std::make_unique<btDbvtBroadphase>();
    m_Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_DynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_Dispatcher.get(), m_OverlappingPairCache.get(), m_Solver.get(), m_CollisionConfig.get());

    m_DynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

    m_DynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

    m_OwnedDebugDrawer = std::make_unique<BulletDebugDrawer>();
    m_OwnedDebugDrawer->Initialize();
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

void BulletPhysicsWorld::SetSolverIterations(int iterations)
{
    if (m_DynamicsWorld)
    {
        m_DynamicsWorld->getSolverInfo().m_numIterations = iterations;
    }
}

void BulletPhysicsWorld::SetCCDEnabled(bool enabled, float threshold)
{
    m_CCDEnabled = enabled;
    m_CCDThreshold = threshold;
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
    btRigidBody* rawBody = bBody->GetRaw();
    if (rawBody)
    {
        // 1. Manually clear manifolds associated with this body from the dispatcher
        btDispatcher* dispatcher = m_DynamicsWorld->getDispatcher();
        if (dispatcher) {
            for (int i = dispatcher->getNumManifolds() - 1; i >= 0; i--) {
                btPersistentManifold* manifold = dispatcher->getManifoldByIndexInternal(i);
                if (manifold && (manifold->getBody0() == rawBody || manifold->getBody1() == rawBody)) {
                    dispatcher->clearManifold(manifold);
                }
            }
        }

        // 2. Clear from broadphase pair cache - MUST be done while handle is valid
        btBroadphaseProxy* handle = rawBody->getBroadphaseHandle();
        auto pairCache = m_DynamicsWorld->getPairCache();
        if (handle && pairCache && dispatcher) {
            pairCache->cleanProxyFromPairs(handle, dispatcher);
        }

        // 3. Remove from dynamics world
        m_DynamicsWorld->removeRigidBody(rawBody);
    }
}

void BulletPhysicsWorld::AddCharacterController(ICharacterController* controller)
{
    if (!m_DynamicsWorld || !controller) return;

    BulletCharacterController* bCC = static_cast<BulletCharacterController*>(controller);
    if (bCC->GetRawController() && bCC->GetGhostObject())
    {
        m_DynamicsWorld->addCollisionObject(bCC->GetGhostObject(), btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
        m_DynamicsWorld->addAction(bCC->GetRawController());
    }
}

void BulletPhysicsWorld::RemoveCharacterController(ICharacterController* controller)
{
    if (!m_DynamicsWorld || !controller) return;

    BulletCharacterController* bCC = static_cast<BulletCharacterController*>(controller);
    if (bCC->GetRawController() && bCC->GetGhostObject())
    {
        m_DynamicsWorld->removeAction(bCC->GetRawController());
        m_DynamicsWorld->removeCollisionObject(bCC->GetGhostObject());
    }
}

void BulletPhysicsWorld::DebugDraw()
{
    if (m_CurrentDebugDrawer)
        m_CurrentDebugDrawer->FrameStart();

    if (m_DynamicsWorld)
    {
        try {
            m_DynamicsWorld->debugDrawWorld();
        } catch (...) {
            LOGGER_ERROR("BulletPhysicsWorld") << "Crash caught during debugDrawWorld";
        }
    }

    if (m_CurrentDebugDrawer)
        m_CurrentDebugDrawer->Flush();
}

RayHit BulletPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, entt::entity ignore)
{
    RayHit hit;
    if (m_DynamicsWorld == nullptr) return hit;

    glm::vec3 end = origin + (dir * maxDist);
    btVector3 btStart(origin.x, origin.y, origin.z);
    btVector3 btEnd(end.x, end.y, end.z);

    IgnoreEntityRayCallback rayCallback(btStart, btEnd, ignore);
    
    try {
        m_DynamicsWorld->rayTest(btStart, btEnd, rayCallback);
    } catch (...) {
        LOGGER_ERROR("BulletPhysicsWorld") << "Crash caught during rayTest";
        return hit;
    }

    if (rayCallback.hasHit())
    {
        hit.hasHit = true;
        hit.distance = rayCallback.m_closestHitFraction * maxDist;
        hit.hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
        hit.hitNormal = glm::vec3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());

        const btCollisionObject* obj = rayCallback.m_collisionObject;
        if (obj && obj->getUserPointer())
        {
            // Offset logic: getUserPointer returns entity + 1
            hit.entity = (entt::entity)((uintptr_t)obj->getUserPointer() - 1);
        }
    }

    return hit;
}

void BulletPhysicsWorld::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    if (m_CurrentDebugDrawer)
    {
        m_CurrentDebugDrawer->drawLine(
            btVector3(from.x, from.y, from.z),
            btVector3(to.x, to.y, to.z),
            btVector3(color.x, color.y, color.z)
        );
    }
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

    if (m_CCDEnabled && mass > 0.0f)
    {
        body->setCcdMotionThreshold(m_CCDThreshold);
        btVector3 center;
        btScalar radius;
        bShape->GetRaw()->getBoundingSphere(center, radius);
        body->setCcdSweptSphereRadius(radius * 0.2f); // Often a fraction of the radius is used
    }

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

std::shared_ptr<ICharacterController> BulletPhysicsWorld::CreateCharacterController(std::shared_ptr<ICollisionShape> shape, float stepHeight)
{
    if (!shape) return nullptr;

    BulletCollisionShape* bShape = static_cast<BulletCollisionShape*>(shape.get());
    
    btPairCachingGhostObject* ghostObject = new btPairCachingGhostObject();
    ghostObject->setCollisionShape(bShape->GetRaw());
    ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    
    btKinematicCharacterController* controller = new btKinematicCharacterController(ghostObject, static_cast<btConvexShape*>(bShape->GetRaw()), stepHeight);

    return std::make_shared<BulletCharacterController>(ghostObject, controller, shape);
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

std::shared_ptr<ICollisionShape> BulletPhysicsWorld::CreateHeightfieldShape(const std::vector<float>& heights, int width, int length, float minHeight, float maxHeight)
{
    if (heights.empty()) {
        LOGGER_ERROR("BulletPhysicsWorld") << "Cannot create heightfield: heights vector is empty.";
        return nullptr;
    }

    LOGGER_INFO("BulletPhysicsWorld") << "Creating Heightfield: " << width << "x" << length << ", range [" << minHeight << ", " << maxHeight << "]";

    // Create wrapper FIRST and move data into it to ensure stable pointer
    auto hData = heights; // Copy to local
    auto wrapper = std::make_shared<BulletHeightfieldCollisionShape>(nullptr, std::move(hData));
    
    // Get stable pointer from the wrapper's member
    float* dataPtr = wrapper->GetHeightDataPointer();

    btHeightfieldTerrainShape* heightfieldShape = new btHeightfieldTerrainShape(
        width, length, dataPtr, 1.0f, minHeight, maxHeight, 1, PHY_FLOAT, false);

    wrapper->SetShape(heightfieldShape);

    LOGGER_INFO("BulletPhysicsWorld") << "btHeightfieldTerrainShape created at " << (void*)heightfieldShape << " with data at " << (void*)dataPtr;

    return wrapper;
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
