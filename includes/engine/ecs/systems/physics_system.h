#pragma once

#include <scene/scene.h>
#include <set>
#include <utility>
#include <entt/entt.hpp>

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
    std::set<CollisionPair> m_activeCollisions;
    bool m_Enabled = true;
};
