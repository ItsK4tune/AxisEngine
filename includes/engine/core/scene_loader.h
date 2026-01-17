#pragma once

#include <string>
#include <vector>
#include <entt/entt.hpp>

class Scene;
class ResourceManager;
class PhysicsWorld;
class SoundManager;
class Application;

class SceneLoader
{
public:
    static std::vector<entt::entity> Load(const std::string& path, Scene& scene, ResourceManager& res, PhysicsWorld& phys, SoundManager& sound, Application* app);
};
