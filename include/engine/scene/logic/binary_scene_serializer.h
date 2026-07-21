#pragma once

#include <core/interface/i_serializer.h>
#include <scene/type/scene_record.h>
#include <cstdint>
#include <string>

struct Scene;

class BinarySceneSerializer : public ISerializer<Scene>
{
public:
    bool Serialize(const std::string& filepath, const Scene& scene) override;
    bool Deserialize(const std::string& filepath, Scene& scene) override;

    bool Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult);

private:
    static constexpr uint32_t VERSION = 5;
};
