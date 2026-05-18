#pragma once

#include <vector>

enum class InputType
{
    Key,
    MouseButton,
    GamepadButton
};

struct InputBinding
{
    InputType type;
    int code;
};

struct InputActionBinding
{
    std::vector<InputBinding> bindings;
};
