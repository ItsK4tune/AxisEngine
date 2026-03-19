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
// --- Lifecycle Events ---

struct EngineInitializedEvent {};
struct EngineShutdownEvent {};

// --- Config Events ---

struct ConfigChangedEvent { 
    enum ChangeType {
        None = 0,
        Graphics = 1 << 0,
        Window = 1 << 1,
        Physics = 1 << 2,
        Audio = 1 << 3,
        Input = 1 << 4,
        General = 1 << 5,
        All = 0xFFFFFFFF
    };
    uint32_t bitmask = All;

    ConfigChangedEvent(uint32_t mask = All) : bitmask(mask) {}
};

// --- Window Events ---

struct WindowResizedEvent { int width; int height; };
struct WindowFocusEvent { bool focused; };

// --- Scene Events ---

struct SceneLoadedEvent { std::string path; };
struct SceneUnloadedEvent { std::string path; };
struct SceneChangedEvent { entt::registry* registry; class Scene* scene; };

// --- Render Events ---

struct RenderFrameBeginEvent { float dt; };
struct RenderFrameEndEvent {};

#include <core/type/resource_events.h>
