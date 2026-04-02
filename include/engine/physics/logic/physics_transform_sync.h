#pragma once

#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/logic/cached_query.h>
#include <ecs/unit/render_components.h>
#include <glm/glm.hpp>
#include <unordered_map>

class IPhysicsWorld;
struct Scene;

#define GLM_ENABLE_EXPERIMENTAL

class PhysicsTransformSync
{
public:
    PhysicsTransformSync(Scene& scene, IPhysicsWorld& physics);
    ~PhysicsTransformSync() = default;

    void Initialize();
    void SyncToPhysics();
    void SyncFromPhysics();

    void SyncTransformToPhysics(entt::entity entity);
    void SyncPhysicsToTransform(entt::entity entity);

private:
    void OnComponentChanged(entt::registry& registry, entt::entity entity);

    Scene& m_Scene;
    IPhysicsWorld& m_Physics;

    CachedQuery<RigidBodyComponent, PositionComponent, RotationComponent, WorldTransformComponent> m_simulationQuery;
    std::unordered_map<entt::entity, uint32_t> m_LastSyncedVersions;
};
