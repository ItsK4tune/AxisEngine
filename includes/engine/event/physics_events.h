#pragma once
#include <entt/entt.hpp>

enum class CollisionEventType
{
    Enter,
    Stay,
    Exit
};

struct EntityCollisionEvent
{
    entt::entity entityA;
    entt::entity entityB;
    CollisionEventType type;
};

struct EntityTriggerEvent
{
    entt::entity entityA;
    entt::entity entityB;
    CollisionEventType type;
};
