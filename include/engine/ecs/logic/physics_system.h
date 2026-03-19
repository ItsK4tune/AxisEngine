#pragma once

#include <ecs/interface/i_system.h>
#include <ecs/logic/cached_query.h>
#include <entt/entt.hpp>
#include <memory>
#include <render/interface/i_render_state_manager.h>
#include <scene/logic/scene.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

class IPhysicsWorld;
class PhysicsCollisionDispatcher;
class PhysicsTransformSync;
class Shader;
class IRenderStateManager;

class PhysicsSystem : public IUpdateSystem
{
public:

    void Initialize() override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 10; }
    std::string GetName() const override { return "PhysicsSystem"; }
    PhysicsSystem();
    ~PhysicsSystem();
    IPhysicsWorld& GetPhysicsWorld() const { return *m_LastPhysicsWorld; }

    void Update(Scene &scene, float dt) override;
    void RenderDebug(Scene &scene, IPhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;
    void Reset();
    void OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity);
    void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity);

    // Raycasting API
    struct RayHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance);
    struct RayHit Raycast(const glm::vec3& start, const glm::vec3& end);
    struct RayHit Raycast(const glm::vec3& origin, float yaw, float pitch, float distance);
    struct RayHit RaycastFromScreen(const glm::vec2& screenPos, float distance = 1000.0f);

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