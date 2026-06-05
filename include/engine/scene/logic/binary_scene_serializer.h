#pragma once

#include <audio/interface/i_audio_source.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/type/scene_record.h>
#include <string>

class BinarySceneSerializer
{
public:
    static bool Save(const std::string& path, Scene& scene);
    static bool Load(const std::string& path, Scene& scene);

private:
    static const uint32_t MAGIC = 0x41585342;
    static const uint32_t VERSION = 3;
};
