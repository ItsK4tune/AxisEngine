#include <engine/ecs/cached_query.h>
#include <engine/ecs/component.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <unordered_map>

class Scene;
class IPhysicsWorld;

class PhysicsTransformSync
{
public:
    PhysicsTransformSync(Scene& scene, IPhysicsWorld& physics);
    ~PhysicsTransformSync();

    void Init();
    void SyncToPhysics();
    void SyncFromPhysics();

    void SyncTransformToPhysics(entt::entity entity);
    void SyncPhysicsToTransform(entt::entity entity);

private:
    void OnComponentChanged(entt::registry& registry, entt::entity entity);

    glm::mat4 GetCachedWorldMatrix(entt::entity entity);

    Scene& m_Scene;
    IPhysicsWorld& m_Physics;

    CachedQuery<RigidBodyComponent, WorldTransformComponent> m_simulationQuery;
    CachedQuery<CharacterControllerComponent, WorldTransformComponent> m_ccQuery;
    std::unordered_map<entt::entity, glm::mat4> m_worldMatrixCache;
    std::unordered_map<entt::entity, uint32_t> m_LastSyncedVersions;
    bool m_initialized = false;
};
