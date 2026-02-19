#pragma once

#include <scene/scene.h>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <entt/entt.hpp>
#include <engine/ecs/cached_query.h>
#include <memory>

#include <interface/graphic/i_render_state_manager.h>

class IPhysicsWorld;
class PhysicsTransformSync;
class PhysicsCollisionDispatcher;

class PhysicsSystem
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(Scene &scene, IPhysicsWorld &physicsWorld, float dt);
    void RenderDebug(Scene &scene, IPhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }
    void Reset();

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;
    
    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair& p) const {
            return std::hash<uint32_t>()((uint32_t)p.first) ^ 
                   (std::hash<uint32_t>()((uint32_t)p.second) << 1);
        }
    };
    


private:
    std::unique_ptr<PhysicsTransformSync> m_transformSync;
    std::unique_ptr<PhysicsCollisionDispatcher> m_collisionDispatcher;

    bool m_Enabled = true;

    Scene *m_LastScene = nullptr;
    IPhysicsWorld *m_LastPhysicsWorld = nullptr;

    mutable entt::entity m_cachedPrimaryCamera = entt::null;
};
