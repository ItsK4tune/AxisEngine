#pragma once

#include <core/interface/i_serializer.h>
#include <scene/type/scene_record.h>
#include <cstdint>
#include <string>

struct Scene;

struct BinarySceneLoadLimits
{
    uintmax_t maxFileBytes = 64ull * 1024ull * 1024ull;
    uint32_t maxPayloadBytes = 64u * 1024u * 1024u;
    uint32_t maxStringBytes = 1024u * 1024u;
    uint32_t maxEntities = 100000u;
};

class BinarySceneSerializer : public ISerializer<Scene>
{
public:
    bool Serialize(const std::string& filepath, const Scene& scene) override;
    bool Deserialize(const std::string& filepath, Scene& scene) override;

    bool Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult);
    bool Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult,
                     const BinarySceneLoadLimits& limits);

private:
    static constexpr uint32_t VERSION = 5;
};
