#include <physic/physics_transform_sync.h>
#include <scene/scene.h>
#include <physic/physic_world.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

PhysicsTransformSync::PhysicsTransformSync(Scene &scene, PhysicsWorld &physics)
    : m_Scene(scene), m_Physics(physics)
{
}

PhysicsTransformSync::~PhysicsTransformSync()
{
}

void PhysicsTransformSync::Init()
{
    if (m_initialized)
        return;

    m_simulationQuery.Update(m_Scene.registry);
    m_initialized = true;
}

glm::mat4 PhysicsTransformSync::GetCachedWorldMatrix(entt::entity entity)
{
    auto it = m_worldMatrixCache.find(entity);
    if (it != m_worldMatrixCache.end())
        return it->second;

    glm::mat4 result;
    if (m_Scene.registry.all_of<TransformComponent>(entity))
    {
        const auto &tc = m_Scene.registry.get<TransformComponent>(entity);
        glm::mat4 local = tc.GetLocalModelMatrix();

        if (m_Scene.registry.valid(tc.parent))
        {
            glm::mat4 parentWorld = GetCachedWorldMatrix(tc.parent);
            result = parentWorld * local;
        }
        else
        {
            result = local;
        }
    }
    else
    {
        result = glm::mat4(1.0f);
    }

    m_worldMatrixCache[entity] = result;
    return result;
}

void PhysicsTransformSync::SyncToPhysics()
{
    m_simulationQuery.Update(m_Scene.registry);
    m_worldMatrixCache.clear();

    const auto &entities = m_simulationQuery.GetEntities();

    for (auto entity : entities)
    {
        auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
        auto &transform = m_Scene.registry.get<TransformComponent>(entity);

        if (!rb.body)
            continue;

        bool isDynamic = !rb.body->isStaticOrKinematicObject();
        bool isKinematic = rb.body->isKinematicObject();
        bool hasParent = m_Scene.registry.valid(transform.parent);

        if ((!isDynamic || isKinematic) && hasParent)
        {
            glm::mat4 worldMatrix = GetCachedWorldMatrix(entity);
            glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
            glm::quat worldRot = glm::quat_cast(worldMatrix);

            btTransform tr;
            tr.setIdentity();
            tr.setOrigin(BulletGLMHelpers::convert(worldPos));
            tr.setRotation(BulletGLMHelpers::convert(worldRot));

            rb.body->setWorldTransform(tr);
            if (rb.body->getMotionState())
            {
                rb.body->getMotionState()->setWorldTransform(tr);
            }
        }
    }
}

void PhysicsTransformSync::SyncFromPhysics()
{
    const auto &entities = m_simulationQuery.GetEntities();

    for (auto entity : entities)
    {
        auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
        auto &transform = m_Scene.registry.get<TransformComponent>(entity);

        if (!rb.body)
            continue;

        bool isDynamic = !rb.body->isStaticOrKinematicObject();
        bool hasParent = m_Scene.registry.valid(transform.parent);

        if (isDynamic && rb.body->isActive())
        {
            btTransform trans;
            if (rb.body->getMotionState())
                rb.body->getMotionState()->getWorldTransform(trans);
            else
                trans = rb.body->getWorldTransform();

            glm::vec3 worldPos = BulletGLMHelpers::convert(trans.getOrigin());
            glm::quat worldRot = BulletGLMHelpers::convert(trans.getRotation());

            if (hasParent && rb.isParentMatter)
            {
                if (m_Scene.registry.all_of<TransformComponent>(transform.parent))
                {
                    glm::mat4 parentWorldMatrix = GetCachedWorldMatrix(transform.parent);
                    glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                    glm::mat4 localMatrix = glm::inverse(parentWorldMatrix) * validWorldMatrix;

                    glm::vec3 s, t, skew;
                    glm::quat r;
                    glm::vec4 perspective;
                    glm::decompose(localMatrix, s, r, t, skew, perspective);

                    transform.position = t;
                    transform.rotation = r;
                }
            }
            else
            {
                transform.position = worldPos;
                transform.rotation = worldRot;
            }
        }
    }
}

void PhysicsTransformSync::SyncTransformToPhysics(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;

    if (!m_Scene.registry.all_of<RigidBodyComponent, TransformComponent>(entity))
        return;

    auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
    auto &transform = m_Scene.registry.get<TransformComponent>(entity);

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

    auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
    auto &transform = m_Scene.registry.get<TransformComponent>(entity);

    if (!rb.body)
        return;

    if (rb.body->isStaticObject())
        return;

    btTransform physicsTransform;
    if (rb.body->getMotionState())
        rb.body->getMotionState()->getWorldTransform(physicsTransform);
    else
        physicsTransform = rb.body->getWorldTransform();

    glm::vec3 position = BulletGLMHelpers::convert(physicsTransform.getOrigin());
    glm::quat rotation = BulletGLMHelpers::convert(physicsTransform.getRotation());

    if (m_Scene.registry.valid(transform.parent))
    {
        if (m_Scene.registry.all_of<TransformComponent>(transform.parent))
        {
            glm::mat4 parentWorldMatrix = m_Scene.registry.get<TransformComponent>(transform.parent).GetWorldModelMatrix(m_Scene.registry);
            glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
            glm::mat4 localMatrix = glm::inverse(parentWorldMatrix) * validWorldMatrix;

            glm::vec3 s, t, skew;
            glm::quat r;
            glm::vec4 perspective;
            glm::decompose(localMatrix, s, r, t, skew, perspective);

            transform.position = t;
            transform.rotation = r;
        }
    }
    else
    {
        transform.position = position;
        transform.rotation = rotation;
    }
}
