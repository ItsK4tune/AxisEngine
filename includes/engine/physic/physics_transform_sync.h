#include <unordered_map>
#include <engine/ecs/cached_query.h>
#include <engine/ecs/component.h>
#include <glm/glm.hpp>

class Scene;
class PhysicsWorld;

class PhysicsTransformSync
{
public:
    PhysicsTransformSync(Scene& scene, PhysicsWorld& physics);
    ~PhysicsTransformSync();

    void Init();
    void SyncToPhysics();
    void SyncFromPhysics();
    
    // Legacy support (optional)
    void SyncTransformToPhysics(entt::entity entity);
    void SyncPhysicsToTransform(entt::entity entity);

private:
    glm::mat4 GetCachedWorldMatrix(entt::entity entity);

    Scene& m_Scene;
    PhysicsWorld& m_Physics;

    CachedQuery<RigidBodyComponent, TransformComponent> m_simulationQuery;
    std::unordered_map<entt::entity, glm::mat4> m_worldMatrixCache;
    bool m_initialized = false;
};
