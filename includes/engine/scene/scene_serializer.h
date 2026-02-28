#pragma once

#include <string>
#include <memory>
#include <vector>
#include <entt/entt.hpp>
#include <app/config_loader.h>

class Scene;
class ResourceManager;
class IPhysicsWorld;
class SoundPlayer;
class Application;

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

class SceneSerializer
{
public:
    static SceneLoadResult Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app);
};
