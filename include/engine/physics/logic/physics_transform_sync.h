#pragma once

#include <ecs/logic/cached_query.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <glm/glm.hpp>
#include <unordered_map>

class IPhysicsWorld;
struct Scene;


class PhysicsTransformSync
{
public:
    PhysicsTransformSync(Scene& scene, IPhysicsWorld& physics);
    ~PhysicsTransformSync();

    void Initialize();
    void SyncToPhysics();
    void SyncFromPhysics();

private:
    Scene& m_Scene;
    IPhysicsWorld& m_Physics;

    CachedQuery<RigidBodyComponent, PositionComponent, RotationComponent, WorldTransformComponent> m_simulationQuery;
    std::unordered_map<entt::entity, uint32_t> m_LastSyncedVersions;

    void OnRigidBodyDestroyed(entt::registry&, entt::entity entity);
};
