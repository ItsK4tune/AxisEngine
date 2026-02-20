#pragma once

#include <string>
#include <vector>
#include <entt/entt.hpp>

class Scene;
class ResourceManager;
class IPhysicsWorld;
class SoundPlayer;
class Application;

class SceneLoader
{
public:
    static std::vector<entt::entity> Load(const std::string& path, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app);
};
