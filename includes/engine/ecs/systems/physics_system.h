#pragma once

#include <scene/scene.h>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <entt/entt.hpp>
#include <engine/ecs/cached_query.h>

class PhysicsWorld;
class Shader;

class PhysicsSystem
{
public:
    void Update(Scene &scene, PhysicsWorld &physicsWorld, float dt);
    void RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;
    
    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair& p) const {
            return std::hash<uint32_t>()((uint32_t)p.first) ^ 
                   (std::hash<uint32_t>()((uint32_t)p.second) << 1);
        }
    };
    
    std::unordered_set<CollisionPair, CollisionPairHash> m_activeCollisions;
    bool m_Enabled = true;
    
    mutable entt::entity m_cachedPrimaryCamera = entt::null;

    CachedQuery<RigidBodyComponent, TransformComponent> m_simulationQuery;
    bool m_signalsConnected = false;
    std::vector<entt::connection> m_connections;

    std::unordered_map<entt::entity, glm::mat4> m_worldMatrixCache;
    
    glm::mat4 GetCachedWorldMatrix(entt::entity entity, entt::registry& registry);
};
