#pragma once

#include <core/logic/config_loader.h>
#include <core/unit/engine_context.h>
#include <entt/entt.hpp>
#include <memory>
#include <scene/type/scene_record.h>
#include <string>
#include <vector>

class IPhysicsWorld;
class ResourceManager;
class Scene;
class SoundPlayer;

class SceneSerializer
{
public:
    static SceneLoadResult Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, EngineContext ctx);
};