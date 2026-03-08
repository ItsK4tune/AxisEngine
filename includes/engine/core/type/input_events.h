#pragma once

#include <string>

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