#include <physic/physics_transform_sync.h>
#include <scene/scene.h>
#include <interface/physics/i_physics_world.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <utils/logger.h>

PhysicsTransformSync::PhysicsTransformSync(Scene &scene, IPhysicsWorld &physics)
    : m_Scene(scene), m_Physics(physics)
{
}

PhysicsTransformSync::~PhysicsTransformSync()
{
    if (m_initialized) {
        m_Scene.registry.on_construct<RigidBodyComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<RigidBodyComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_construct<TransformComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<TransformComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
    }
}

void PhysicsTransformSync::Init()
{
    if (m_initialized)
        return;

    m_Scene.registry.on_construct<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_construct<TransformComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<TransformComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);

    m_simulationQuery.Update(m_Scene.registry);
    m_initialized = true;
}

void PhysicsTransformSync::OnComponentChanged(entt::registry& registry, entt::entity entity)
{
    m_simulationQuery.MarkDirty();
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

        rb.body->SetUserPointer((void*)(uintptr_t)entity);

        uint32_t currentVersion = transform.GetVersion();
        if (m_LastSyncedVersions.find(entity) != m_LastSyncedVersions.end() &&
            m_LastSyncedVersions[entity] == currentVersion)
        {
            continue;
        }

        bool isDynamic = !rb.body->IsStatic() && !rb.body->IsKinematic();
        bool isKinematic = rb.body->IsKinematic();
        bool hasParent = m_Scene.registry.valid(transform.parent);

        glm::mat4 worldMatrix = GetCachedWorldMatrix(entity);
        glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
        glm::quat worldRot = glm::quat_cast(worldMatrix);

        rb.body->SetWorldTransform(worldPos, worldRot);

        m_LastSyncedVersions[entity] = currentVersion;
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

        bool isDynamic = !rb.body->IsStatic() && !rb.body->IsKinematic();
        bool hasParent = m_Scene.registry.valid(transform.parent);

        // Special case: for dynamic bodies, we sync even if inactive during the first few frames
        // to ensure the initial render state matches the physics state.
        if (isDynamic && (rb.body->IsActive() || m_LastSyncedVersions.find(entity) == m_LastSyncedVersions.end()))
        {
            glm::vec3 worldPos;
            glm::quat worldRot;
            rb.body->GetWorldTransform(worldPos, worldRot);

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

            transform.GetLocalModelMatrix();
            m_LastSyncedVersions[entity] = transform.GetVersion();
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

    rb.body->SetWorldTransform(position, rotation);

    rb.body->SetLinearVelocity(glm::vec3(0, 0, 0));
    rb.body->SetAngularVelocity(glm::vec3(0, 0, 0));
    rb.body->Activate();
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

    if (rb.body->IsStatic())
        return;

    glm::vec3 position;
    glm::quat rotation;
    rb.body->GetWorldTransform(position, rotation);

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
