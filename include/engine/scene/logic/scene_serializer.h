#pragma once

#include <core/interface/i_serializer.h>
#include <core/logic/yaml_parser.h>
#include <scene/type/scene_record.h>
#include <entt/entt.hpp>
#include <memory>
#include <iosfwd>
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
    bool DeserializeFromString(const std::string& content, const std::string& sourceName, Scene& scene,
                               SceneLoadResult& outResult);
    std::string SerializeToString(const Scene& scene, const std::string& sceneName = "",
                                  bool includeTransient = false);
    std::string SerializeEntitiesToString(const Scene& scene, const std::vector<entt::entity>& roots,
                                          const std::string& stripNamePrefix = "",
                                          bool includeTransient = false);

    static std::string NormalizeSceneName(const std::string& name);
    static std::string NormalizePath(const std::string& path);

private:
    bool DeserializeNodes(std::vector<YAMLNode> roots, const std::string& sourceName, const std::string& sourceDisplay,
                          Scene& scene, SceneLoadResult& outResult);
    bool SerializeToStream(std::ostream& stream, const Scene& scene, const std::string& sceneName,
                           bool includeTransient = false);

    ResourceManager& m_Res;
    IPhysicsWorld* m_Phys;
    AudioService* m_Audio;
};
