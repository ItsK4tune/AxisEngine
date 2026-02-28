#pragma once

#include <functional>
#include <string>

enum class InputEvent
{
    Pressed,
    Held,
    Released
};

struct KeyBinding
{
    int key;
    InputEvent event;
    std::function<void()> callback;
};
