#pragma once

#include <string>
#include <core/logic/logger_types.h>
#include <core/type/app_config.h>

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
    uint32_t entityA; // Use raw ID or opaque type for L2
    uint32_t entityB;
    CollisionEventType type;
};

struct EntityTriggerEvent
{
    uint32_t entityA;
    uint32_t entityB;
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
    AppConfig config;

    ConfigChangedEvent(const AppConfig& cfg, uint32_t mask = All) 
        : config(cfg), bitmask(mask) {}
};

// --- Window Events ---

struct WindowResizedEvent { int width; int height; };
struct WindowFocusEvent { bool focused; };

// --- Render Events ---

struct RenderFrameBeginEvent { float dt; };
struct RenderFrameEndEvent {};
