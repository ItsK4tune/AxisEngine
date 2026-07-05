#pragma once

#include <audio/interface/i_audio_source.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/type/scene_record.h>
#include <core/interface/i_serializer.h>
#include <string>

class BinarySceneSerializer : public ISerializer<Scene>
{
public:
    bool Serialize(const std::string& filepath, const Scene& scene) override;
    bool Deserialize(const std::string& filepath, Scene& scene) override;

    bool Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult);

private:
    static const uint32_t MAGIC = 0x41585342;
    static const uint32_t VERSION = 4;
};
