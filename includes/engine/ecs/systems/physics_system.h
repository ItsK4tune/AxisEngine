#pragma once

#include <scene/scene.h>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <entt/entt.hpp>
#include <engine/ecs/cached_query.h>
#include <future>

class PhysicsWorld;
class Shader;
class PhysicsTransformSync;
class PhysicsCollisionDispatcher;

class PhysicsSystem
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(Scene &scene, PhysicsWorld &physicsWorld, float dt);
    void RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }
    void SetAsyncPhysics(bool async) { m_AsyncPhysics = async; }

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;
    
    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair& p) const {
            return std::hash<uint32_t>()((uint32_t)p.first) ^ 
                   (std::hash<uint32_t>()((uint32_t)p.second) << 1);
        }
    };
    
    void WaitAsyncPhysics();

private:
    std::unique_ptr<PhysicsTransformSync> m_transformSync;
    std::unique_ptr<PhysicsCollisionDispatcher> m_collisionDispatcher;

    std::future<void> m_physicsFuture;
    bool m_AsyncPhysics = true;
    bool m_Enabled = true;

    mutable entt::entity m_cachedPrimaryCamera = entt::null;
};
