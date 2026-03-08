#pragma once

#include <audio/interface/i_audio_source.h>
#include <core/unit/engine_context.h>
#include <physics/interface/i_physics_world.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>
#include <string>

class BinarySceneSerializer
{
public:
    static bool Serialize(const std::string& filepath, Scene& scene);
    static bool Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, EngineContext ctx);

private:
    static const uint32_t MAGIC = 0x41585342;
    static const uint32_t VERSION = 1;
};