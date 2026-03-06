#include <systems/physics/physics_transform_sync.h>
#include <scene/scene.h>
#include <systems/physics/interfaces/i_physics_world.h>
#include <ecs/component.h>
#include <core/utils/bullet_glm_helpers.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <core/utils/logger.h>

PhysicsTransformSync::PhysicsTransformSync(Scene &scene, IPhysicsWorld &physics)
    : m_Scene(scene), m_Physics(physics)
{
}

PhysicsTransformSync::~PhysicsTransformSync()
{
    if (m_initialized)
    {
        m_Scene.registry.on_construct<RigidBodyComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<RigidBodyComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_construct<PositionComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<PositionComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
    }
}

void PhysicsTransformSync::Init()
{
    if (m_initialized)
        return;

    m_Scene.registry.on_construct<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_construct<PositionComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<PositionComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);

    m_simulationQuery.Update(m_Scene.registry);
    m_initialized = true;
}

void PhysicsTransformSync::OnComponentChanged(entt::registry &registry, entt::entity entity)
{
    m_simulationQuery.MarkDirty();
}

void PhysicsTransformSync::SyncToPhysics()
{
    m_simulationQuery.Update(m_Scene.registry);

    const auto &entities = m_simulationQuery.GetEntities();

    for (auto entity : entities)
    {
        auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);
        if (!world || !rb.body) continue;

        rb.body->SetUserPointer((void *)(uintptr_t)entity);

        uint32_t currentVersion = world->version;
        if (m_LastSyncedVersions.find(entity) != m_LastSyncedVersions.end() &&
            m_LastSyncedVersions[entity] == currentVersion)
        {
            continue;
        }

        glm::vec3 worldPos = glm::vec3(world->worldMatrix[3]);
        glm::quat worldRot = glm::quat_cast(world->worldMatrix);

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
        auto* pos = m_Scene.registry.try_get<PositionComponent>(entity);
        auto* rot = m_Scene.registry.try_get<RotationComponent>(entity);
        auto* hier = m_Scene.registry.try_get<HierarchyComponent>(entity);
        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);

        if (!rb.body || !pos || !rot || !world) continue;

        bool isDynamic = !rb.body->IsStatic() && !rb.body->IsKinematic();
        bool hasParent = (hier && hier->parent != entt::null);

        if (isDynamic && (rb.body->IsActive() || m_LastSyncedVersions.find(entity) == m_LastSyncedVersions.end()))
        {
            glm::vec3 worldPos;
            glm::quat worldRot;
            rb.body->GetWorldTransform(worldPos, worldRot);

            if (hasParent && rb.isParentMatter)
            {
                if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
                {
                    glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                    glm::mat4 localMatrix = glm::inverse(parentWorld->worldMatrix) * validWorldMatrix;

                    glm::vec3 s, t, skew;
                    glm::quat r;
                    glm::vec4 perspective;
                    glm::decompose(localMatrix, s, r, t, skew, perspective);

                    pos->value = t;
                    rot->value = r;
                }
            }
            else
            {
                pos->value = worldPos;
                rot->value = worldRot;
            }

            world->isDirty = true;
            m_LastSyncedVersions[entity] = world->version;
            
            // Legacy support
            if (auto* legacy = m_Scene.registry.try_get<TransformComponent>(entity))
            {
                legacy->position = pos->value;
                legacy->rotation = rot->value;
            }
        }
    }
}

void PhysicsTransformSync::SyncTransformToPhysics(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity)) return;

    auto* rb = m_Scene.registry.try_get<RigidBodyComponent>(entity);
    auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);

    if (!rb || !rb->body || !world) return;

    glm::vec3 position = glm::vec3(world->worldMatrix[3]);
    glm::quat rotation = glm::quat_cast(world->worldMatrix);

    rb->body->SetWorldTransform(position, rotation);
    rb->body->SetLinearVelocity(glm::vec3(0, 0, 0));
    rb->body->SetAngularVelocity(glm::vec3(0, 0, 0));
    rb->body->Activate();
}

void PhysicsTransformSync::SyncPhysicsToTransform(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity)) return;

    auto* rb = m_Scene.registry.try_get<RigidBodyComponent>(entity);
    auto* pos = m_Scene.registry.try_get<PositionComponent>(entity);
    auto* rot = m_Scene.registry.try_get<RotationComponent>(entity);
    auto* hier = m_Scene.registry.try_get<HierarchyComponent>(entity);
    auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);

    if (!rb || !rb->body || !pos || !rot || !world) return;
    if (rb->body->IsStatic()) return;

    glm::vec3 position;
    glm::quat rotation;
    rb->body->GetWorldTransform(position, rotation);

    if (hier && m_Scene.registry.valid(hier->parent))
    {
        if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
        {
            glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
            glm::mat4 localMatrix = glm::inverse(parentWorld->worldMatrix) * validWorldMatrix;

            glm::vec3 s, t, skew;
            glm::quat r;
            glm::vec4 perspective;
            glm::decompose(localMatrix, s, r, t, skew, perspective);

            pos->value = t;
            rot->value = r;
        }
    }
    else
    {
        pos->value = position;
        rot->value = rotation;
    }
    
    world->isDirty = true;
    
    // Legacy support
    if (auto* legacy = m_Scene.registry.try_get<TransformComponent>(entity))
    {
        legacy->position = pos->value;
        legacy->rotation = rot->value;
    }
}
