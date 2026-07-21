#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/logic/cached_query.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <core/logic/event_manager.h>
#include <core/interface/i_optimization_configurable.h>
#include <render/interface/i_render_state_manager.h>
#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

class IPhysicsWorld;
class ICollisionShape;
class PhysicsCollisionDispatcher;
class PhysicsTransformSync;
class Shader;
class IRenderStateManager;
class Model;

class PhysicsSystem : public IUpdateSystem,
                      public IRenderSystem,
                      public IECSSystem,
                      public IOptimizationConfigurable
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Initialize() override;
    void Shutdown() override;
    void Reset() override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 10;
    }
    std::string GetName() const override
    {
        return "PhysicsSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Physics | SystemCategory::Update;
    }
    bool WantsFixedUpdate() const override
    {
        return true;
    }
    IPhysicsWorld& GetPhysicsWorld() const
    {
        return *m_LastPhysicsWorld;
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void Update(Scene& scene, float dt) override;
    void FixedUpdate(Scene& scene, float fixedDt) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;
    void RenderDebug(Scene& scene, Shader& shader, int screenWidth, int screenHeight,
                     IRenderStateManager& renderState) override;

    void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity);
    void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity);
    void OnShapeConstructed(entt::registry& registry, entt::entity entity);
    void OnShapeDestroyed(entt::registry& registry, entt::entity entity);
    void OnShapeUpdated(entt::registry& registry, entt::entity entity);
    void InitializeRigidBodyDirect(Scene& scene, entt::entity entity, struct RigidShapeComponent& shape,
                                   struct RigidBodyComponent& rb, IPhysicsWorld& physics);

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;

    struct CollisionPairHash
    {
        std::size_t operator()(const CollisionPair& p) const
        {
            return std::hash<uint32_t>()((uint32_t)p.first) ^ (std::hash<uint32_t>()((uint32_t)p.second) << 1);
        }
    };

    std::unique_ptr<PhysicsTransformSync> m_transformSync;
    std::unique_ptr<PhysicsCollisionDispatcher> m_collisionDispatcher;

    bool m_Enabled = true;

    void Step(Scene& scene, float dt);

    Scene* m_LastScene = nullptr;
    IPhysicsWorld* m_LastPhysicsWorld = nullptr;

    std::unordered_set<entt::entity> m_PendingRigidBodies;
    CachedQuery<CharacterControllerComponent, InfoComponent> m_ControllerQuery;
    struct MeshShapeCacheKey
    {
        const Model* model = nullptr;
        glm::vec3 scale{1.0f};
        bool operator==(const MeshShapeCacheKey&) const = default;
    };
    struct MeshShapeCacheHash
    {
        size_t operator()(const MeshShapeCacheKey& key) const
        {
            size_t seed = std::hash<const Model*>{}(key.model);
            const auto combine = [&seed](float value) {
                seed ^= std::hash<float>{}(value) + 0x9e3779b9U + (seed << 6) + (seed >> 2);
            };
            combine(key.scale.x);
            combine(key.scale.y);
            combine(key.scale.z);
            return seed;
        }
    };
    std::unordered_map<MeshShapeCacheKey, std::weak_ptr<ICollisionShape>, MeshShapeCacheHash> m_MeshShapeCache;
    bool m_MeshShapeCacheEnabled = true;
    EventSubscriptionList m_EventSubscriptions;
};
