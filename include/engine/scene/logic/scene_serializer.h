#pragma once

#include <core/interface/i_serializer.h>
#include <core/logic/config_loader.h>
#include <scene/type/scene_record.h>
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>

class IPhysicsWorld;
class ResourceManager;
struct Scene;
class AudioService;

class SceneSerializer : public ISerializer<Scene>
{
public:
    SceneSerializer(ResourceManager& res, IPhysicsWorld* phys, AudioService* audio);

    bool Serialize(const std::string& filepath, const Scene& scene) override;
    bool Deserialize(const std::string& filepath, Scene& scene) override;

    bool Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult);
    bool Serialize(const std::string& filepath, const Scene& scene, const std::string& sceneName);

    static std::string NormalizeSceneName(const std::string& name);
    static std::string NormalizePath(const std::string& path);

private:
    ResourceManager& m_Res;
    IPhysicsWorld* m_Phys;
    AudioService* m_Audio;
};
