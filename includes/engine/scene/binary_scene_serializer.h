#pragma once

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <systems/physics/interfaces/i_physics_world.h>
#include <systems/audio/interfaces/i_audio_source.h>
#include <core/engine_context.h>
#include <string>

class BinarySceneSerializer
{
public:
    static bool Serialize(const std::string& filepath, Scene& scene);
    static bool Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, EngineContext ctx);

private:
    static const uint32_t MAGIC = 0x41585342; // "AXSB"
    static const uint32_t VERSION = 1;
};
