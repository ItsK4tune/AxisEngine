#include <unordered_map>
#include <engine/ecs/cached_query.h>
#include <engine/ecs/component.h>
#include <glm/glm.hpp>

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
    glm::mat4 GetCachedWorldMatrix(entt::entity entity);

    Scene& m_Scene;
    IPhysicsWorld& m_Physics;

    CachedQuery<RigidBodyComponent, TransformComponent> m_simulationQuery;
    std::unordered_map<entt::entity, glm::mat4> m_worldMatrixCache;
    std::unordered_map<entt::entity, uint32_t> m_LastSyncedVersions;
    bool m_initialized = false;
};
