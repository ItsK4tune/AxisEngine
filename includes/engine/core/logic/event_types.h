#pragma once

#include <string>
#include <entt/entt.hpp>

// --- Input Events ---

struct InputActionPressedEvent { std::string actionName; };
struct InputActionReleasedEvent { std::string actionName; };
struct InputActionHeldEvent { std::string actionName; };

struct KeyPressedEvent { int key; int mods; };
struct KeyReleasedEvent { int key; int mods; };

struct MouseMovedEvent { double x; double y; };
struct MouseButtonPressedEvent { int button; int mods; };
struct MouseButtonReleasedEvent { int button; int mods; };
struct MouseScrolledEvent { double xOffset; double yOffset; };

// --- Physics Events ---

enum class CollisionEventType { Enter, Stay, Exit };

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

#include <core/type/resource_events.h>
