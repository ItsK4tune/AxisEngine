#pragma once

#include <string>
#include <vector>
#include <entt/entt.hpp>
#include <app/config_loader.h>

struct SceneRecord
{
    std::string name;
    std::string filePath;
    int  loadOrder = 0;
    bool persistent = false;
    bool inviolable = false;    // true for load.axs - cannot be unloaded by any method

    std::vector<entt::entity> entities;

    std::vector<std::string> ownedShaders;
    std::vector<std::string> ownedModels;
    std::vector<std::string> ownedTextures;
    std::vector<std::string> ownedFonts;
    std::vector<std::string> ownedSkyboxes;
    std::vector<std::string> ownedAnimations;
    std::vector<std::string> ownedSounds;

    AppConfig appliedConfig;
    bool hasConfig = false;
};

struct SceneLoadResult
{
    std::vector<entt::entity> entities;

    std::vector<std::string> loadedShaders;
    std::vector<std::string> loadedModels;
    std::vector<std::string> loadedTextures;
    std::vector<std::string> loadedFonts;
    std::vector<std::string> loadedSkyboxes;
    std::vector<std::string> loadedAnimations;
    std::vector<std::string> loadedSounds;

    AppConfig appliedConfig;
    bool hasConfig = false;
};
