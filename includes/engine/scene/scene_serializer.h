#pragma once

#include <string>
#include <memory>
#include <vector>
#include <entt/entt.hpp>
#include <core/app/config_loader.h>
#include <scene/types/scene_record.h>

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
