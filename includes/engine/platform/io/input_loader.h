#pragma once
#include <string>

class InputManager;

/**
 * @brief Utilities for loading input bindings from files.
 */
class InputLoader
{
public:
    /**
     * @brief Loads key/button bindings from a file into the given manager.
     * @param filepath Path to the bindings file.
     * @param inputManager The manager to populate.
     * @return true if loading succeeded.
     */
    static bool LoadBindings(const std::string& filepath, InputManager& inputManager);
};
