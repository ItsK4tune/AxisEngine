#include <physic/physics_transform_sync.h>
#include <scene/scene.h>
#include <physic/physic_world.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

PhysicsTransformSync::PhysicsTransformSync(Scene& scene, PhysicsWorld& physics)
    : m_Scene(scene), m_Physics(physics)
{
}

PhysicsTransformSync::~PhysicsTransformSync()
{
}

void PhysicsTransformSync::SyncTransformToPhysics(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent, TransformComponent>(entity))
        return;

    auto& rb = m_Scene.registry.get<RigidBodyComponent>(entity);
    auto& transform = m_Scene.registry.get<TransformComponent>(entity);

    if (!rb.body)
        return;

    glm::mat4 worldMatrix = transform.GetWorldModelMatrix(m_Scene.registry);
    glm::vec3 position = glm::vec3(worldMatrix[3]);
    glm::quat rotation = glm::quat_cast(worldMatrix);

    btTransform tr;
    tr.setIdentity();
    tr.setOrigin(BulletGLMHelpers::convert(position));
    tr.setRotation(BulletGLMHelpers::convert(rotation));

    rb.body->setWorldTransform(tr);
    if (rb.body->getMotionState())
    {
        rb.body->getMotionState()->setWorldTransform(tr);
    }

    rb.body->setLinearVelocity(btVector3(0, 0, 0));
    rb.body->setAngularVelocity(btVector3(0, 0, 0));
    rb.body->activate();
}

void PhysicsTransformSync::SyncPhysicsToTransform(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent, TransformComponent>(entity))
        return;

    auto& rb = m_Scene.registry.get<RigidBodyComponent>(entity);
    auto& transform = m_Scene.registry.get<TransformComponent>(entity);

    if (!rb.body)
        return;

    if (rb.body->isStaticObject())
        return;

    btTransform physicsTransform;
    rb.body->getMotionState()->getWorldTransform(physicsTransform);

    glm::vec3 position = BulletGLMHelpers::convert(physicsTransform.getOrigin());
    glm::quat rotation = BulletGLMHelpers::convert(physicsTransform.getRotation());

    transform.position = position;
    transform.rotation = rotation;
}

void PhysicsTransformSync::SyncAllTransformsToPhysics()
{
    auto view = m_Scene.registry.view<RigidBodyComponent, TransformComponent>();
    
    for (auto entity : view)
    {
        SyncTransformToPhysics(entity);
    }
}

void PhysicsTransformSync::SyncAllPhysicsToTransforms()
{
    auto view = m_Scene.registry.view<RigidBodyComponent, TransformComponent>();
    
    for (auto entity : view)
    {
        SyncPhysicsToTransform(entity);
    }
}

void PhysicsTransformSync::SetupPhysicsConstraints(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent, TransformComponent>(entity))
        return;

    auto& rb = m_Scene.registry.get<RigidBodyComponent>(entity);
    auto& transform = m_Scene.registry.get<TransformComponent>(entity);

    if (!rb.body || !rb.isAttachedToParent)
        return;

    if (!m_Scene.registry.valid(transform.parent))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent>(transform.parent))
        return;

    auto& parentRb = m_Scene.registry.get<RigidBodyComponent>(transform.parent);
    if (!parentRb.body)
        return;

    btTransform frameInA, frameInB;

    btTransform parentWorldTrans = parentRb.body->getWorldTransform();
    btTransform childWorldTrans = rb.body->getWorldTransform();

    frameInA = parentWorldTrans.inverse() * childWorldTrans;
    frameInB.setIdentity();

    btFixedConstraint* fixedConstraint = new btFixedConstraint(
        *parentRb.body,
        *rb.body,
        frameInA,
        frameInB);

    m_Physics.AddConstraint(fixedConstraint);
    rb.constraint = fixedConstraint;
}

void PhysicsTransformSync::RemovePhysicsConstraints(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent>(entity))
        return;

    auto& rb = m_Scene.registry.get<RigidBodyComponent>(entity);

    if (rb.constraint)
    {
        m_Physics.RemoveConstraint(rb.constraint);
        delete rb.constraint;
        rb.constraint = nullptr;
    }
}
