#pragma once

#include <string>
#include <memory>
#include <vector>
#include <entt/entt.hpp>
#include <app/config_loader.h>
#include <scene/scene_types.h>

class Scene;
class ResourceManager;
class IPhysicsWorld;
class SoundPlayer;
#include <core/engine_context.h>

class SceneSerializer
{
public:
    static SceneLoadResult Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, EngineContext ctx);
};
