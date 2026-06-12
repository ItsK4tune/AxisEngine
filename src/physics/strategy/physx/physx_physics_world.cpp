#include <physics/strategy/physx/physx_physics_world.h>
#include <core/logic/backend_factory_registry.h>
#include <core/logic/logger.h>
#include <extensions/PxRigidActorExt.h>
#include <extensions/PxDistanceJoint.h>
#include <extensions/PxFixedJoint.h>
#include <extensions/PxRevoluteJoint.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxBoxController.h>
#include <cooking/PxCooking.h>
#include <common/PxTolerancesScale.h>
#include <foundation/PxIO.h>
#include <algorithm>

// --- PhysXRigidBody Implementation ---

void PhysXRigidBody::SetLinearVelocity(const glm::vec3& vel)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setLinearVelocity(physx::PxVec3(vel.x, vel.y, vel.z));
    }
}

glm::vec3 PhysXRigidBody::GetLinearVelocity() const
{
    if (!m_Actor) return glm::vec3(0.0f);
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        physx::PxVec3 vel = dynamicBody->getLinearVelocity();
        return glm::vec3(vel.x, vel.y, vel.z);
    }
    return glm::vec3(0.0f);
}

void PhysXRigidBody::SetAngularVelocity(const glm::vec3& vel)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setAngularVelocity(physx::PxVec3(vel.x, vel.y, vel.z));
    }
}

glm::vec3 PhysXRigidBody::GetAngularVelocity() const
{
    if (!m_Actor) return glm::vec3(0.0f);
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        physx::PxVec3 vel = dynamicBody->getAngularVelocity();
        return glm::vec3(vel.x, vel.y, vel.z);
    }
    return glm::vec3(0.0f);
}

void PhysXRigidBody::ApplyCentralForce(const glm::vec3& force)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->addForce(physx::PxVec3(force.x, force.y, force.z), physx::PxForceMode::eFORCE);
    }
}

void PhysXRigidBody::ApplyCentralImpulse(const glm::vec3& impulse)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->addForce(physx::PxVec3(impulse.x, impulse.y, impulse.z), physx::PxForceMode::eIMPULSE);
    }
}

void PhysXRigidBody::ApplyTorque(const glm::vec3& torque)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->addTorque(physx::PxVec3(torque.x, torque.y, torque.z));
    }
}

void PhysXRigidBody::SetFriction(float friction)
{
    if (!m_Actor) return;
    physx::PxU32 numShapes = m_Actor->getNbShapes();
    std::vector<physx::PxShape*> shapes(numShapes);
    m_Actor->getShapes(shapes.data(), numShapes);
    for (auto* shape : shapes)
    {
        physx::PxMaterial* mat = nullptr;
        shape->getMaterials(&mat, 1);
        if (mat)
        {
            mat->setStaticFriction(friction);
            mat->setDynamicFriction(friction);
        }
    }
}

void PhysXRigidBody::SetRestitution(float restitution)
{
    if (!m_Actor) return;
    physx::PxU32 numShapes = m_Actor->getNbShapes();
    std::vector<physx::PxShape*> shapes(numShapes);
    m_Actor->getShapes(shapes.data(), numShapes);
    for (auto* shape : shapes)
    {
        physx::PxMaterial* mat = nullptr;
        shape->getMaterials(&mat, 1);
        if (mat)
            mat->setRestitution(restitution);
    }
}

void PhysXRigidBody::SetWorldTransform(const glm::vec3& pos, const glm::quat& rot)
{
    if (!m_Actor) return;
    physx::PxTransform trans(physx::PxVec3(pos.x, pos.y, pos.z), physx::PxQuat(rot.x, rot.y, rot.z, rot.w));
    m_Actor->setGlobalPose(trans);
}

void PhysXRigidBody::GetWorldTransform(glm::vec3& pos, glm::quat& rot) const
{
    if (!m_Actor)
    {
        pos = glm::vec3(0.0f);
        rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    physx::PxTransform trans = m_Actor->getGlobalPose();
    pos = glm::vec3(trans.p.x, trans.p.y, trans.p.z);
    rot = glm::quat(trans.q.w, trans.q.x, trans.q.y, trans.q.z);
}

void PhysXRigidBody::Activate(bool forceActivation)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        if (forceActivation || dynamicBody->isSleeping())
        {
            dynamicBody->wakeUp();
        }
    }
}

bool PhysXRigidBody::IsActive() const
{
    if (!m_Actor) return false;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        return !dynamicBody->isSleeping();
    }
    return true;
}

bool PhysXRigidBody::IsStatic() const
{
    if (!m_Actor) return false;
    return m_Actor->is<physx::PxRigidStatic>() != nullptr;
}

bool PhysXRigidBody::IsKinematic() const
{
    if (!m_Actor) return false;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        return (dynamicBody->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) ? true : false;
    }
    return false;
}

bool PhysXRigidBody::IsTrigger() const
{
    if (!m_Actor) return false;
    physx::PxU32 numShapes = m_Actor->getNbShapes();
    if (numShapes > 0)
    {
        std::vector<physx::PxShape*> shapes(numShapes);
        m_Actor->getShapes(shapes.data(), numShapes);
        return (shapes[0]->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE) ? true : false;
    }
    return false;
}

void PhysXRigidBody::SetKinematic(bool isKinematic)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
    }
}

void PhysXRigidBody::SetStatic(bool isStatic)
{
    (void)isStatic;
}

void PhysXRigidBody::SetTrigger(bool isTrigger)
{
    if (!m_Actor) return;
    physx::PxU32 numShapes = m_Actor->getNbShapes();
    std::vector<physx::PxShape*> shapes(numShapes);
    m_Actor->getShapes(shapes.data(), numShapes);
    for (auto* shape : shapes)
    {
        if (isTrigger)
        {
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
        }
        else
        {
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, false);
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
        }
    }
}

void PhysXRigidBody::SetAlwaysActive(bool alwaysActive)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        if (alwaysActive)
        {
            dynamicBody->setSleepThreshold(0.0f);
            dynamicBody->wakeUp();
        }
        else
        {
            dynamicBody->setSleepThreshold(0.05f);
        }
    }
}

void PhysXRigidBody::SetLinearFactor(const glm::vec3& factor)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, factor.x < 0.5f);
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, factor.y < 0.5f);
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, factor.z < 0.5f);
    }
}

void PhysXRigidBody::SetAngularFactor(const glm::vec3& factor)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, factor.x < 0.5f);
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, factor.y < 0.5f);
        dynamicBody->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, factor.z < 0.5f);
    }
}

void PhysXRigidBody::SetDamping(float linearDamping, float angularDamping)
{
    if (!m_Actor) return;
    if (auto* dynamicBody = m_Actor->is<physx::PxRigidDynamic>())
    {
        dynamicBody->setLinearDamping(linearDamping);
        dynamicBody->setAngularDamping(angularDamping);
    }
}


// --- PhysXCharacterController Implementation ---

void PhysXCharacterController::SetWalkDirection(const glm::vec3& dir)
{
    m_WalkDirection = dir;
}

void PhysXCharacterController::SetVelocity(const glm::vec3& vel, float timeInterval)
{
    m_WalkDirection = vel;
    (void)timeInterval;
}

void PhysXCharacterController::SetFallSpeed(float speed)
{
    m_FallSpeed = speed;
}

void PhysXCharacterController::SetGravity(const glm::vec3& gravity)
{
    m_Gravity = gravity;
}

void PhysXCharacterController::SetJumpSpeed(float speed)
{
    m_JumpSpeed = speed;
}

void PhysXCharacterController::Jump()
{
    if (m_OnGround)
    {
        m_VerticalVelocity = m_JumpSpeed;
        m_OnGround = false;
    }
}

bool PhysXCharacterController::OnGround() const
{
    return m_OnGround;
}

void PhysXCharacterController::SetStepHeight(float height)
{
    if (m_Controller)
        m_Controller->setStepOffset(height);
}

void PhysXCharacterController::SetMaxSlope(float slopeRadians)
{
    if (m_Controller)
        m_Controller->setSlopeLimit(std::cos(slopeRadians));
}

void PhysXCharacterController::GetWorldTransform(glm::vec3& pos, glm::quat& rot) const
{
    if (m_Controller)
    {
        physx::PxExtendedVec3 pxPos = m_Controller->getPosition();
        pos = glm::vec3(pxPos.x, pxPos.y, pxPos.z);
    }
    else
    {
        pos = glm::vec3(0.0f);
    }
    rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void PhysXCharacterController::SetWorldTransform(const glm::vec3& pos, const glm::quat& rot)
{
    if (m_Controller)
    {
        m_Controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));
    }
    (void)rot;
}

void PhysXCharacterController::Activate(bool forceActivation)
{
    (void)forceActivation;
}

void PhysXCharacterController::SetUserPointer(void* ptr)
{
    if (m_Controller && m_Controller->getActor())
    {
        m_Controller->getActor()->userData = ptr;
    }
}

void* PhysXCharacterController::GetUserPointer() const
{
    if (m_Controller && m_Controller->getActor())
    {
        return m_Controller->getActor()->userData;
    }
    return nullptr;
}

void PhysXCharacterController::Update(float dt)
{
    if (!m_Controller) return;

    m_VerticalVelocity += m_Gravity.y * dt;
    if (m_VerticalVelocity < -m_FallSpeed)
        m_VerticalVelocity = -m_FallSpeed;

    glm::vec3 disp = m_WalkDirection * dt;
    disp.y += m_VerticalVelocity * dt;

    physx::PxControllerFilters filters;
    physx::PxControllerCollisionFlags flags = m_Controller->move(
        physx::PxVec3(disp.x, disp.y, disp.z), 0.001f, dt, filters);

    m_OnGround = (flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN) ? true : false;
    if (m_OnGround && m_VerticalVelocity < 0.0f)
    {
        m_VerticalVelocity = 0.0f;
    }
}


// --- PhysXPhysicsWorld Implementation ---

PhysXPhysicsWorld::PhysXPhysicsWorld()
{
}

PhysXPhysicsWorld::~PhysXPhysicsWorld()
{
    Clear();
}

void PhysXPhysicsWorld::Initialize()
{
    m_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
    if (!m_Foundation)
    {
        LOGGER_ERROR("PhysX") << "Failed to create PxFoundation.";
        return;
    }

    m_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_Foundation, physx::PxTolerancesScale(), true);
    if (!m_Physics)
    {
        LOGGER_ERROR("PhysX") << "Failed to create PxPhysics.";
        return;
    }

    physx::PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    m_CpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
    sceneDesc.cpuDispatcher = m_CpuDispatcher;
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
    
    m_Scene = m_Physics->createScene(sceneDesc);
    if (!m_Scene)
    {
        LOGGER_ERROR("PhysX") << "Failed to create PxScene.";
        return;
    }

    m_DefaultMaterial = m_Physics->createMaterial(0.5f, 0.5f, 0.6f);
    m_ControllerManager = PxCreateControllerManager(*m_Scene);
    LOGGER_INFO("PhysX") << "PhysX initialized successfully.";
}

void PhysXPhysicsWorld::Update(float dt)
{
    if (!m_Scene) return;

    m_Accumulator += dt;
    if (m_Accumulator < m_FixedTimeStep)
        return;

    m_Accumulator -= m_FixedTimeStep;

    for (auto* controller : m_CharacterControllers)
    {
        if (controller)
            controller->Update(m_FixedTimeStep);
    }

    m_Scene->simulate(m_FixedTimeStep);
    m_Scene->fetchResults(true);
}

void PhysXPhysicsWorld::Clear()
{
    if (m_ControllerManager)
    {
        m_ControllerManager->release();
        m_ControllerManager = nullptr;
    }
    if (m_Scene)
    {
        m_Scene->release();
        m_Scene = nullptr;
    }
    if (m_CpuDispatcher)
    {
        m_CpuDispatcher->release();
        m_CpuDispatcher = nullptr;
    }
    if (m_Physics)
    {
        m_Physics->release();
        m_Physics = nullptr;
    }
    if (m_Foundation)
    {
        m_Foundation->release();
        m_Foundation = nullptr;
    }
}

void PhysXPhysicsWorld::SetGravity(const glm::vec3& gravity)
{
    if (m_Scene)
        m_Scene->setGravity(physx::PxVec3(gravity.x, gravity.y, gravity.z));
}

void PhysXPhysicsWorld::SetMode(int mode)
{
    (void)mode;
}

void PhysXPhysicsWorld::SetSimulationSettings(float fixedTimeStep, int maxSubSteps)
{
    m_FixedTimeStep = fixedTimeStep > 0.0f ? fixedTimeStep : (1.0f / 60.0f);
    (void)maxSubSteps;
}

void PhysXPhysicsWorld::SetSolverIterations(int iterations)
{
    (void)iterations;
}

void PhysXPhysicsWorld::SetCCDEnabled(bool enabled, float threshold)
{
    (void)enabled;
    (void)threshold;
}

void PhysXPhysicsWorld::AddRigidBody(IRigidBody* body)
{
    if (!m_Scene || !body) return;
    auto* pxBody = static_cast<PhysXRigidBody*>(body);
    if (pxBody->GetRaw())
    {
        m_Scene->addActor(*pxBody->GetRaw());
    }
}

void PhysXPhysicsWorld::RemoveRigidBody(IRigidBody* body)
{
    if (!m_Scene || !body) return;
    auto* pxBody = static_cast<PhysXRigidBody*>(body);
    if (pxBody->GetRaw())
    {
        m_Scene->removeActor(*pxBody->GetRaw());
    }
}

void PhysXPhysicsWorld::AddCharacterController(ICharacterController* controller)
{
    if (!controller) return;
    auto* pxController = static_cast<PhysXCharacterController*>(controller);
    auto it = std::find(m_CharacterControllers.begin(), m_CharacterControllers.end(), pxController);
    if (it == m_CharacterControllers.end())
    {
        m_CharacterControllers.push_back(pxController);
    }
}

void PhysXPhysicsWorld::RemoveCharacterController(ICharacterController* controller)
{
    if (!controller) return;
    auto* pxController = static_cast<PhysXCharacterController*>(controller);
    auto it = std::find(m_CharacterControllers.begin(), m_CharacterControllers.end(), pxController);
    if (it != m_CharacterControllers.end())
    {
        m_CharacterControllers.erase(it);
    }
}

void PhysXPhysicsWorld::AddConstraint(std::shared_ptr<IConstraint> constraint)
{
    (void)constraint;
}

void PhysXPhysicsWorld::RemoveConstraint(std::shared_ptr<IConstraint> constraint)
{
    (void)constraint;
}

void PhysXPhysicsWorld::DebugDraw()
{
}

RayHit PhysXPhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, entt::entity ignore)
{
    RayHit hit;
    if (!m_Scene) return hit;

    physx::PxRaycastBuffer hitBuffer;
    physx::PxQueryFilterData filterData;
    bool status = m_Scene->raycast(physx::PxVec3(origin.x, origin.y, origin.z), physx::PxVec3(dir.x, dir.y, dir.z), maxDist, hitBuffer, physx::PxHitFlag::eDEFAULT, filterData);
    if (status && hitBuffer.hasBlock)
    {
        hit.hasHit = true;
        hit.distance = hitBuffer.block.distance;
        hit.hitPoint = glm::vec3(hitBuffer.block.position.x, hitBuffer.block.position.y, hitBuffer.block.position.z);
        hit.hitNormal = glm::vec3(hitBuffer.block.normal.x, hitBuffer.block.normal.y, hitBuffer.block.normal.z);
        if (hitBuffer.block.actor && hitBuffer.block.actor->userData)
        {
            entt::entity hitEnt = (entt::entity)((uintptr_t)hitBuffer.block.actor->userData - 1);
            if (hitEnt != ignore)
                hit.entity = hitEnt;
            else
                hit.hasHit = false;
        }
    }
    return hit;
}

void PhysXPhysicsWorld::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    (void)from;
    (void)to;
    (void)color;
}

void PhysXPhysicsWorld::SyncRigidBody(IRigidBody* body, const glm::vec3& pos, const glm::quat& rot)
{
    if (!body) return;
    auto* pxBody = static_cast<PhysXRigidBody*>(body);
    if (pxBody->GetRaw())
    {
        physx::PxTransform trans(physx::PxVec3(pos.x, pos.y, pos.z), physx::PxQuat(rot.x, rot.y, rot.z, rot.w));
        pxBody->GetRaw()->setGlobalPose(trans);
    }
}

void PhysXPhysicsWorld::SetCollisionFilter(CollisionFilterCallback callback)
{
    m_FilterCallback = callback;
}

std::shared_ptr<IRigidBody> PhysXPhysicsWorld::CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape)
{
    if (!m_Physics || !shape) return nullptr;

    physx::PxTransform trans(physx::PxVec3(startPos.x, startPos.y, startPos.z), physx::PxQuat(startRot.x, startRot.y, startRot.z, startRot.w));
    physx::PxRigidActor* actor = nullptr;

    auto* pxShape = static_cast<PhysXCollisionShape*>(shape.get());

    if (mass > 0.0f)
    {
        physx::PxRigidDynamic* dynamicActor = m_Physics->createRigidDynamic(trans);
        physx::PxRigidActorExt::createExclusiveShape(*dynamicActor, pxShape->GetGeometry(), *m_DefaultMaterial);
        dynamicActor->setMass(mass);
        actor = dynamicActor;
    }
    else
    {
        physx::PxRigidStatic* staticActor = m_Physics->createRigidStatic(trans);
        physx::PxRigidActorExt::createExclusiveShape(*staticActor, pxShape->GetGeometry(), *m_DefaultMaterial);
        actor = staticActor;
    }

    return std::make_shared<PhysXRigidBody>(actor, shape);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateBoxShape(const glm::vec3& halfExtents)
{
    return std::make_shared<PhysXCollisionShape>(physx::PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z), CollisionShapeType::Box);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateSphereShape(float radius)
{
    return std::make_shared<PhysXCollisionShape>(physx::PxSphereGeometry(radius), CollisionShapeType::Sphere);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateCapsuleShape(float radius, float height)
{
    return std::make_shared<PhysXCollisionShape>(physx::PxCapsuleGeometry(radius, height * 0.5f), CollisionShapeType::Capsule);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateCylinderShape(float radius, float height)
{
    // PhysX has no built-in cylinder; we approximate it with a box.
    return std::make_shared<PhysXCollisionShape>(physx::PxBoxGeometry(radius, height * 0.5f, radius), CollisionShapeType::Cylinder);
}

std::shared_ptr<ICharacterController> PhysXPhysicsWorld::CreateCharacterController(std::shared_ptr<ICollisionShape> shape, float stepHeight)
{
    if (!m_ControllerManager || !shape) return nullptr;

    auto* pxShape = static_cast<PhysXCollisionShape*>(shape.get());
    physx::PxController* controller = nullptr;

    if (shape->GetType() == CollisionShapeType::Capsule)
    {
        const auto& geom = static_cast<const physx::PxCapsuleGeometry&>(pxShape->GetGeometry());
        physx::PxCapsuleControllerDesc desc;
        desc.height = geom.halfHeight * 2.0f;
        desc.radius = geom.radius;
        desc.stepOffset = stepHeight;
        desc.material = m_DefaultMaterial;
        controller = m_ControllerManager->createController(desc);
    }
    else
    {
        physx::PxBoxControllerDesc desc;
        desc.halfHeight = 1.0f;
        desc.halfSideExtent = 0.5f;
        desc.halfForwardExtent = 0.5f;
        desc.stepOffset = stepHeight;
        desc.material = m_DefaultMaterial;
        controller = m_ControllerManager->createController(desc);
    }

    return std::make_shared<PhysXCharacterController>(controller, shape);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateCompoundShape()
{
    // Return an empty geometry holder representing a compound shape.
    return std::make_shared<PhysXCollisionShape>(physx::PxBoxGeometry(0.1f, 0.1f, 0.1f), CollisionShapeType::CompoundHull);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateMeshShape(const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
{
    if (vertices.empty() || indices.empty() || !m_Physics) return nullptr;

    physx::PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = vertices.size() / 3;
    meshDesc.points.stride = sizeof(float) * 3;
    meshDesc.points.data = vertices.data();
    meshDesc.triangles.count = indices.size() / 3;
    meshDesc.triangles.stride = sizeof(uint32_t) * 3;
    meshDesc.triangles.data = indices.data();

    physx::PxDefaultMemoryOutputStream writeBuffer;
    physx::PxTriangleMeshCookingResult::Enum result;
    bool status = PxCookTriangleMesh(m_Physics->getTolerancesScale(), meshDesc, writeBuffer, &result);
    if (!status)
        return nullptr;

    physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    physx::PxTriangleMesh* triangleMesh = m_Physics->createTriangleMesh(readBuffer);
    return std::make_shared<PhysXCollisionShape>(physx::PxTriangleMeshGeometry(triangleMesh), CollisionShapeType::Mesh);
}

std::shared_ptr<ICollisionShape> PhysXPhysicsWorld::CreateHeightfieldShape(const std::vector<float>& heights, int width, int length, float minHeight, float maxHeight)
{
    (void)heights;
    (void)width;
    (void)length;
    (void)minHeight;
    (void)maxHeight;
    // Fallback to simple box.
    return CreateBoxShape(glm::vec3(width, 1.0f, length));
}

void PhysXPhysicsWorld::AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child, const glm::vec3& pos, const glm::quat& rot)
{
    (void)parent;
    (void)child;
    (void)pos;
    (void)rot;
}

std::shared_ptr<IConstraint> PhysXPhysicsWorld::CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB)
{
    if (!rbA || !rbB) return nullptr;
    auto* pxA = static_cast<PhysXRigidBody*>(rbA.get());
    auto* pxB = static_cast<PhysXRigidBody*>(rbB.get());
    physx::PxJoint* joint = physx::PxDistanceJointCreate(*m_Physics, pxA->GetRaw(), physx::PxTransform(physx::PxVec3(pivotInA.x, pivotInA.y, pivotInA.z)), pxB->GetRaw(), physx::PxTransform(physx::PxVec3(pivotInB.x, pivotInB.y, pivotInB.z)));
    return std::make_shared<PhysXConstraint>(joint);
}

std::shared_ptr<IConstraint> PhysXPhysicsWorld::CreateFixedConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::quat& rotInA, const glm::quat& rotInB)
{
    if (!rbA || !rbB) return nullptr;
    auto* pxA = static_cast<PhysXRigidBody*>(rbA.get());
    auto* pxB = static_cast<PhysXRigidBody*>(rbB.get());
    physx::PxTransform transA(physx::PxVec3(pivotInA.x, pivotInA.y, pivotInA.z), physx::PxQuat(rotInA.x, rotInA.y, rotInA.z, rotInA.w));
    physx::PxTransform transB(physx::PxVec3(pivotInB.x, pivotInB.y, pivotInB.z), physx::PxQuat(rotInB.x, rotInB.y, rotInB.z, rotInB.w));
    physx::PxJoint* joint = physx::PxFixedJointCreate(*m_Physics, pxA->GetRaw(), transA, pxB->GetRaw(), transB);
    return std::make_shared<PhysXConstraint>(joint);
}

std::shared_ptr<IConstraint> PhysXPhysicsWorld::CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::vec3& axisInA, const glm::vec3& axisInB)
{
    if (!rbA || !rbB) return nullptr;
    auto* pxA = static_cast<PhysXRigidBody*>(rbA.get());
    auto* pxB = static_cast<PhysXRigidBody*>(rbB.get());
    physx::PxTransform transA(physx::PxVec3(pivotInA.x, pivotInA.y, pivotInA.z), physx::PxQuat(axisInA.x, axisInA.y, axisInA.z, 1.0f));
    physx::PxTransform transB(physx::PxVec3(pivotInB.x, pivotInB.y, pivotInB.z), physx::PxQuat(axisInB.x, axisInB.y, axisInB.z, 1.0f));
    physx::PxJoint* joint = physx::PxRevoluteJointCreate(*m_Physics, pxA->GetRaw(), transA, pxB->GetRaw(), transB);
    return std::make_shared<PhysXConstraint>(joint);
}

std::vector<CollisionInfo> PhysXPhysicsWorld::GetActiveCollisions()
{
    return m_ActiveCollisions;
}

namespace axis::backend
{
void RegisterPhysXPhysicsBackendFactories()
{
    BackendFactoryRegistry::RegisterPhysics(
        PhysicsBackend::PhysX, [](const AppConfig&) { return std::make_unique<PhysXPhysicsWorld>(); });
}
} // namespace axis::backend
