#pragma once

#include <string>

// --- Logical Action Events --- //
// These events are fired based on user-defined Bindings (e.g. "Jump", "Fire")

struct InputActionPressedEvent
{
    std::string actionName;
};

struct InputActionReleasedEvent
{
    std::string actionName;
};

struct InputActionHeldEvent
{
    std::string actionName;
};

// --- Hardware Input Events --- //
// These events are fired directly by the OS window for raw input processing

struct KeyPressedEvent
{
    int key;
    int mods;
};

struct KeyReleasedEvent
{
    int key;
    int mods;
};

struct MouseMovedEvent
{
    double x;
    double y;
};

struct MouseButtonPressedEvent
{
    int button;
    int mods;
};

struct MouseButtonReleasedEvent
{
    int button;
    int mods;
};

struct MouseScrolledEvent
{
    double xOffset;
    double yOffset;
};
