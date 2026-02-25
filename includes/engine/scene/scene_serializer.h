#pragma once

#include <string>
#include <memory>
#include <vector>
#include <entt/entt.hpp>

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

class SceneSerializer
{
public:
    static std::vector<entt::entity> Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app);
    static void Serialize(Scene& scene, const std::string& filepath);

private:
    static std::vector<YAMLNode> ParseAXS(const std::string& filepath);
    static void WriteAXS(std::ofstream& out, const YAMLNode& node, int indent);
};
