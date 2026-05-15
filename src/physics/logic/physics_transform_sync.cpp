#include <physics/logic/physics_transform_sync.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <core/logic/logger.h>

PhysicsTransformSync::PhysicsTransformSync(Scene &scene, IPhysicsWorld &physics)
    : m_Scene(scene), m_Physics(physics)
{
}

void PhysicsTransformSync::Initialize()
{
    m_simulationQuery.Update(m_Scene.registry);
}

void PhysicsTransformSync::OnComponentChanged(entt::registry& registry, entt::entity entity)
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
        if (!rb.body) continue;

        auto* info = m_Scene.registry.try_get<InfoComponent>(entity);
        if (info && !info->isActive) {
            if (rb.body->IsActive()) rb.body->Activate(false);
            continue;
        }

        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);
        if (!world) continue;

        uint32_t currentVersion = world->version;
        if (m_LastSyncedVersions.find(entity) != m_LastSyncedVersions.end() && m_LastSyncedVersions[entity] == currentVersion)
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

        rb.body->SetWorldTransform(worldPos, worldRot);
        if (!rb.body->IsKinematic() && !rb.body->IsStatic()) {
            rb.body->SetLinearVelocity(glm::vec3(0.0f));
            rb.body->SetAngularVelocity(glm::vec3(0.0f));
        }
        rb.body->Activate();
        m_LastSyncedVersions[entity] = currentVersion;
    }
}

void PhysicsTransformSync::SyncFromPhysics()
{
    m_simulationQuery.Update(m_Scene.registry);
    const auto &entities = m_simulationQuery.GetEntities();

    for (auto entity : entities)
    {
        auto &rb = m_Scene.registry.get<RigidBodyComponent>(entity);
        auto* pos = m_Scene.registry.try_get<PositionComponent>(entity);
        auto* rot = m_Scene.registry.try_get<RotationComponent>(entity);
        auto* hier = m_Scene.registry.try_get<HierarchyComponent>(entity);
        auto* world = m_Scene.registry.try_get<WorldTransformComponent>(entity);
        auto* scl = m_Scene.registry.try_get<ScaleComponent>(entity);

        if (!rb.body || !pos || !rot || !world || !scl) continue;
        
        auto* info = m_Scene.registry.try_get<InfoComponent>(entity);
        if (info && !info->isActive) continue;
        
        // Anti-snapback: If the user is dragging the object in the editor, skip syncing from physics
        if (world->isDirty) continue;

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
            glm::vec3 Rt, Rs, Rskew; glm::quat Rr; glm::vec4 Rperspective;
            glm::decompose(rootMtx, Rs, Rr, Rt, Rskew, Rperspective);

            if (hasParent) // ALWAYS factor out parent transform to maintain relative ECS coordinate integrity!
            {
                if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
                {
                    glm::mat4 physMtx = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                    glm::mat4 targetMtx = glm::inverse(parentWorld->worldMatrix) * physMtx;

                    glm::vec3 Pt, Ps, Pskew; glm::quat Pr; glm::vec4 Pperspective;
                    glm::decompose(targetMtx, Ps, Pr, Pt, Pskew, Pperspective);

                    rot->value = Pr * glm::inverse(Rr);
                    glm::vec3 scaledRt = scl->value * Rt;
                    pos->value = Pt - (rot->value * scaledRt);

                    glm::mat4 newLocalMtx = glm::translate(glm::mat4(1.0f), pos->value) *
                                            glm::mat4_cast(rot->value) *
                                            glm::scale(glm::mat4(1.0f), scl->value);
                    world->worldMatrix = parentWorld->worldMatrix * newLocalMtx;
                }
            }
            else
            {
                glm::mat4 targetMtx = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
                
                glm::vec3 Pt, Ps, Pskew; glm::quat Pr; glm::vec4 Pperspective;
                glm::decompose(targetMtx, Ps, Pr, Pt, Pskew, Pperspective);

                rot->value = Pr * glm::inverse(Rr);
                glm::vec3 scaledRt = scl->value * Rt;
                pos->value = Pt - (rot->value * scaledRt);

                glm::mat4 newLocalMtx = glm::translate(glm::mat4(1.0f), pos->value) *
                                        glm::mat4_cast(rot->value) *
                                        glm::scale(glm::mat4(1.0f), scl->value);
                world->worldMatrix = newLocalMtx;
            }

            world->isDirty = false;
            m_LastSyncedVersions[entity] = world->version;

            if (hier) {
                for (auto child : hier->children) {
                    if (auto* cWorld = m_Scene.registry.try_get<WorldTransformComponent>(child)) {
                        cWorld->isDirty = true;
                    }
                }
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

    rb->body->SetWorldTransform(worldPos, worldRot);
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

    glm::vec3 worldPos;
    glm::quat worldRot;
    rb->body->GetWorldTransform(worldPos, worldRot);

    glm::mat4 rootMtx = glm::mat4(1.0f);
    if (auto* mesh = m_Scene.registry.try_get<MeshRendererComponent>(entity)) {
        if (mesh->model) {
            rootMtx = mesh->model->GetRootTransform();
        }
    }

    bool hasParent = (hier && hier->parent != entt::null);
    glm::mat4 rootMtxInv = glm::inverse(rootMtx);

    if (hasParent && rb->isParentMatter)
    {
        if (auto* parentWorld = m_Scene.registry.try_get<WorldTransformComponent>(hier->parent))
        {
            glm::mat4 physMtx = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
            glm::mat4 entityMtx = physMtx * rootMtxInv;
            glm::mat4 localMtx = glm::inverse(parentWorld->worldMatrix) * entityMtx;

            glm::vec3 s, t, skew;
            glm::quat r;
            glm::vec4 perspective;
            if (glm::decompose(localMtx, s, r, t, skew, perspective)) {
                rot->value = r;
                pos->value = t;
            }
        }
    }
    else
    {
        glm::mat4 physMtx = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);
        glm::mat4 entityMtx = physMtx * rootMtxInv;
        
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
