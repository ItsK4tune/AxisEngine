#pragma once

#include <entt/entt.hpp>
#include <unordered_set>
#include <vector>

class Scene;
class PhysicsWorld;

struct CollisionPair
{
    entt::entity a;
    entt::entity b;

    bool operator==(const CollisionPair &other) const
    {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

struct CollisionPairHash
{
    std::size_t operator()(const CollisionPair &k) const
    {
        return std::hash<uint32_t>()((uint32_t)k.a) ^ std::hash<uint32_t>()((uint32_t)k.b);
    }
};

class PhysicsCollisionDispatcher
{
public:
    PhysicsCollisionDispatcher(Scene& scene, PhysicsWorld& physics);
    ~PhysicsCollisionDispatcher();

    void DispatchEvents();

private:
    Scene& m_Scene;
    PhysicsWorld& m_Physics;

    std::unordered_set<CollisionPair, CollisionPairHash> m_activeCollisions;
};
