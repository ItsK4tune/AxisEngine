#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/model.h>
#include <physics/logic/physics_transform_sync.h>
#include <scene/logic/scene.h>
#include <physics/interface/i_physics_world.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <core/logic/logger.h>

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
        m_Scene.registry.on_construct<WorldTransformComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<WorldTransformComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_construct<CharacterControllerComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
        m_Scene.registry.on_destroy<CharacterControllerComponent>().disconnect<&PhysicsTransformSync::OnComponentChanged>(this);
    }
}

void PhysicsTransformSync::Initialize()
{
    if (m_initialized)
        return;

    m_Scene.registry.on_construct<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_construct<WorldTransformComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<WorldTransformComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_construct<CharacterControllerComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);
    m_Scene.registry.on_destroy<CharacterControllerComponent>().connect<&PhysicsTransformSync::OnComponentChanged>(this);

    m_simulationQuery.Update(m_Scene.registry);
    m_ccQuery.Update(m_Scene.registry);
    m_initialized = true;
}

void PhysicsTransformSync::OnComponentChanged(entt::registry &registry, entt::entity entity)
{
    m_simulationQuery.MarkDirty();
    m_ccQuery.MarkDirty();
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

        rb.body->SetUserPointer((void *)((uintptr_t)entity + 1));

        uint32_t currentVersion = world->version;
        if (m_LastSyncedVersions.find(entity) != m_LastSyncedVersions.end() &&
            m_LastSyncedVersions[entity] == currentVersion)
        {
            continue;
        }

        glm::mat4 syncMtx = world->worldMatrix;
        if (auto* mesh = m_Scene.registry.try_get<MeshRendererComponent>(entity)) {
            if (mesh->model) {
                syncMtx *= mesh->model->GetRootTransform();
            }
        }

        glm::vec3 worldPos = glm::vec3(syncMtx[3]);
        glm::mat3 m3(syncMtx);
        m3[0] = glm::normalize(m3[0]);
        m3[1] = glm::normalize(m3[1]);
        m3[2] = glm::normalize(m3[2]);
        glm::quat worldRot = glm::quat_cast(m3);

        rb.body->SetWorldTransform(worldPos + worldRot * rb.positionOffset, worldRot * rb.rotationOffset);
        m_LastSyncedVersions[entity] = currentVersion;
    }

    m_ccQuery.Update(m_Scene.registry);
    const auto &ccEntities = m_ccQuery.GetEntities();
    for (auto entity : ccEntities)
    {
        auto &cc = m_Scene.registry.get<CharacterControllerComponent>(entity);
        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);
        if (!world || !cc.controller) continue;

        cc.controller->SetUserPointer((void *)((uintptr_t)entity + 1));

        uint32_t currentVersion = world->version;
        if (m_LastSyncedVersions.find(entity) != m_LastSyncedVersions.end() &&
            m_LastSyncedVersions[entity] == currentVersion)
        {
            continue;
        }

        glm::vec3 worldPos = glm::vec3(world->worldMatrix[3]);
        glm::mat3 m3(world->worldMatrix);
        m3[0] = glm::normalize(m3[0]);
        m3[1] = glm::normalize(m3[1]);
        m3[2] = glm::normalize(m3[2]);
        glm::quat worldRot = glm::quat_cast(m3);

        cc.controller->SetWorldTransform(worldPos, worldRot);
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

            glm::mat4 rootMtx = glm::mat4(1.0f);
            if (auto* mesh = m_Scene.registry.try_get<MeshRendererComponent>(entity)) {
                if (mesh->model) {
                    rootMtx = mesh->model->GetRootTransform();
                }
            }

            if (hasParent && rb.isParentMatter)
            {
                if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
                {



                }
            }
            else
            {
                glm::quat invRotOffset = glm::inverse(rb.rotationOffset);
                glm::quat physWorldRot = worldRot * invRotOffset;
                glm::vec3 physWorldPos = worldPos - physWorldRot * rb.positionOffset;
                


                glm::mat4 physMtx = glm::translate(glm::mat4(1.0f), physWorldPos) * glm::mat4_cast(physWorldRot);
                glm::mat4 entityMtx = physMtx * glm::inverse(rootMtx);
                
                glm::vec3 s, t, skew;
                glm::quat r;
                glm::vec4 perspective;
                if (glm::decompose(entityMtx, s, r, t, skew, perspective)) {
                    rot->value = r;
                    pos->value = t;
                }
            }

            world->isDirty = true;
            m_LastSyncedVersions[entity] = world->version;
            
        }
    }

    const auto &ccEntities = m_ccQuery.GetEntities();
    for (auto entity : ccEntities)
    {
        auto &cc = m_Scene.registry.get<CharacterControllerComponent>(entity);
        auto* pos = m_Scene.registry.try_get<PositionComponent>(entity);
        auto* rot = m_Scene.registry.try_get<RotationComponent>(entity);
        auto* hier = m_Scene.registry.try_get<HierarchyComponent>(entity);
        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);

        if (!cc.controller || !pos || !rot || !world) continue;

        bool hasParent = (hier && hier->parent != entt::null);

        if (m_LastSyncedVersions.find(entity) == m_LastSyncedVersions.end())
        {
            glm::vec3 worldPos;
            glm::quat worldRot;
            cc.controller->GetWorldTransform(worldPos, worldRot);

            if (hasParent)
            {
                if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
                {
                    float det = glm::determinant(parentWorld->worldMatrix);
                    if (std::abs(det) < 0.0001f)
                    {
                        LOGGER_WARN("PhysicsTransformSync") << "Singular matrix detected on parent of CC entity " << (uint32_t)entity << ". Skipping sync.";
                        continue;
                    }

                    glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                    glm::mat4 localMatrix = glm::inverse(parentWorld->worldMatrix) * validWorldMatrix;

                    glm::vec3 s, t, skew;
                    glm::quat r;
                    glm::vec4 perspective;
                    if (glm::decompose(localMatrix, s, r, t, skew, perspective))
                    {
                        if (!glm::any(glm::isnan(t)) && !glm::any(glm::isnan(r)))
                        {
                            pos->value = t;
                            rot->value = r;
                        }
                    }
                }
            }
            else
            {
                pos->value = worldPos;
                rot->value = worldRot;
            }

            world->isDirty = true;
            m_LastSyncedVersions[entity] = world->version;
        }
        else
        {
            glm::vec3 worldPos;
            glm::quat worldRot;
            cc.controller->GetWorldTransform(worldPos, worldRot);

            if (hasParent)
            {
                if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
                {
                    float det = glm::determinant(parentWorld->worldMatrix);
                    if (std::abs(det) < 0.0001f)
                    {
                        LOGGER_WARN("PhysicsTransformSync") << "Singular matrix detected on parent of CC entity " << (uint32_t)entity << ". Skipping sync.";
                        continue;
                    }

                    glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                    glm::mat4 localMatrix = glm::inverse(parentWorld->worldMatrix) * validWorldMatrix;

                    glm::vec3 s, t, skew;
                    glm::quat r;
                    glm::vec4 perspective;
                    if (glm::decompose(localMatrix, s, r, t, skew, perspective))
                    {
                        if (!glm::any(glm::isnan(t)) && !glm::any(glm::isnan(r)))
                        {
                            pos->value = t;
                            rot->value = r;
                        }
                    }
                }
            }
            else
            {
                pos->value = worldPos;
                rot->value = worldRot;
            }

            world->isDirty = true;
            m_LastSyncedVersions[entity] = world->version;
        }
    }
}

void PhysicsTransformSync::SyncTransformToPhysics(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity)) return;

    auto* rb = m_Scene.registry.try_get<RigidBodyComponent>(entity);
    auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);

    if (!rb || !rb->body || !world) return;

    glm::mat4 syncMtx = world->worldMatrix;
    if (auto* mesh = m_Scene.registry.try_get<MeshRendererComponent>(entity)) {
        if (mesh->model) {
            syncMtx *= mesh->model->GetRootTransform();
        }
    }

    glm::vec3 worldPos = glm::vec3(syncMtx[3]);
    glm::mat3 m3(syncMtx);
    m3[0] = glm::normalize(m3[0]);
    m3[1] = glm::normalize(m3[1]);
    m3[2] = glm::normalize(m3[2]);
    glm::quat worldRot = glm::quat_cast(m3);

    rb->body->SetWorldTransform(worldPos + worldRot * rb->positionOffset, worldRot * rb->rotationOffset);
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
            float det = glm::determinant(parentWorld->worldMatrix);
            if (std::abs(det) < 0.0001f)
            {
                LOGGER_WARN("PhysicsTransformSync") << "Singular matrix detected on parent of entity " << (uint32_t)entity << " in direct sync. Skipping.";
                return;
            }

            glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
            glm::mat4 localMatrix = glm::inverse(parentWorld->worldMatrix) * validWorldMatrix;

            glm::vec3 s, t, skew;
            glm::quat r;
            glm::vec4 perspective;
            if (glm::decompose(localMatrix, s, r, t, skew, perspective))
            {
                if (!glm::any(glm::isnan(t)) && !glm::any(glm::isnan(r)))
                {
                    pos->value = t;
                    rot->value = r;
                }
            }
        }
    }
    else
    {
        pos->value = position;
        rot->value = rotation;
    }
    
    world->isDirty = true;
}
