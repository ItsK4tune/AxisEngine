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

struct YAMLNode {
    std::string key;
    std::string value;
    std::vector<YAMLNode> children;

    YAMLNode* GetChild(const std::string& k) {
        for(auto& c : children) {
            if(c.key == k) return &c;
        }
        return nullptr;
    }
    
    std::string GetChildValue(const std::string& k, const std::string& defaultVal = "") const {
        for(auto& c : children) {
            if(c.key == k) return c.value;
        }
        return defaultVal;
    }
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

class SceneSerializer
{
public:
    static SceneLoadResult Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app);

private:
    static std::vector<YAMLNode> ParseAXS(const std::string& filepath);
};
