#pragma once

#include <string>

class InputManager;

class InputLoader
{
public:
    static bool LoadBindings(const std::string& filepath, InputManager& inputManager);
};
