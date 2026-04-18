#pragma once

#include <string>
#include <core/logic/logger_types.h>
#include <core/type/app_config.h>



struct InputActionPressedEvent { std::string actionName; };
struct InputActionReleasedEvent { std::string actionName; };
struct InputActionHeldEvent { std::string actionName; };

struct KeyPressedEvent { int key; int mods; };
struct KeyReleasedEvent { int key; int mods; };

struct MouseMovedEvent { double x; double y; };
struct MouseButtonPressedEvent { int button; int mods; };
struct MouseButtonReleasedEvent { int button; int mods; };
struct MouseScrolledEvent { double xOffset; double yOffset; };



enum class CollisionEventType { Enter, Stay, Exit };

struct EntityCollisionEvent
{
    uint32_t entityA;
    uint32_t entityB;
    CollisionEventType type;
};

struct EntityTriggerEvent
{
    uint32_t entityA;
    uint32_t entityB;
    CollisionEventType type;
};



struct EngineInitializedEvent {};
struct EngineShutdownEvent {};

struct SystemEnabledEvent {
    std::string systemName;
    bool enabled;
};





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



struct WindowResizedEvent { int width; int height; };
struct WindowFocusEvent { bool focused; };



struct RenderFrameBeginEvent { float dt; };
struct RenderFrameEndEvent {};

struct FrameRenderData {
    uint32_t mainFBO = 0;
    int width = 0;
    int height = 0;
    float alpha = 0.0f;
};

struct FrameRenderDataEvent {
    FrameRenderData data;
};

struct DebugNoTextureChangedEvent {
    bool enabled;
};

struct PhysicsDebugRenderEvent {
    struct Scene* scene;
    int width;
    int height;
};
