#pragma once

#include <ecs/i_system.h>

#include <engine/ecs/cached_query.h>
#include <entt/entt.hpp>
#include <rendering/interfaces/i_render_state_manager.h>
#include <memory>
#include <scene/scene.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

class IPhysicsWorld;
class PhysicsTransformSync;
class PhysicsCollisionDispatcher;

class PhysicsSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 10; }
    std::string GetName() const override { return "PhysicsSystem"; }
    PhysicsSystem();
    ~PhysicsSystem();

    void Update(Scene &scene, float dt) override;
    void RenderDebug(Scene &scene, IPhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState);
    void Reset();
    void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity);
    void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity);

private:
    EngineContext m_Ctx;
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
