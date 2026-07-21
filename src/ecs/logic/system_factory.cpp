#include <ecs/logic/system_factory.h>
#include <core/logic/localization_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/camera_system.h>
#include <ecs/logic/decal_system.h>
#include <ecs/logic/fragment_system.h>
#include <ecs/logic/geometry_system.h>
#include <ecs/logic/lighting_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/planar_reflection_system.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/logic/reflection_probe_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/shadow_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/streaming_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/transform_system.h>
#include <ecs/logic/transparent_system.h>
#include <ecs/logic/ui_interact_system.h>
#include <ecs/logic/ui_render_system.h>
#include <ecs/logic/video_system.h>
#include <navigation/logic/navigation_system.h>
#include <network/network_system.h>
#include <mutex>

std::map<std::string, SystemFactory::Creator>& SystemFactory::GetRegistry()
{
    static std::map<std::string, Creator> registry;
    return registry;
}

std::mutex& SystemFactory::GetMutex()
{
    static std::mutex mutex;
    return mutex;
}

void SystemFactory::RegisterDefault(const std::string& name, Creator creator)
{
    std::lock_guard lock(GetMutex());
    GetRegistry().try_emplace(name, std::move(creator));
}

void SystemFactory::EnsureBuiltInSystemsRegistered()
{
    static std::once_flag once;
    std::call_once(once, [] {
#define AXIS_REGISTER_BUILTIN(SystemType) \
    RegisterDefault(#SystemType, [] { return std::make_unique<SystemType>(); })
        AXIS_REGISTER_BUILTIN(AnimationSystem);
        AXIS_REGISTER_BUILTIN(AudioSystem);
        AXIS_REGISTER_BUILTIN(CameraSystem);
        AXIS_REGISTER_BUILTIN(DecalSystem);
        AXIS_REGISTER_BUILTIN(FragmentSystem);
        AXIS_REGISTER_BUILTIN(GeometrySystem);
        AXIS_REGISTER_BUILTIN(LightingSystem);
        AXIS_REGISTER_BUILTIN(LocalizationSystem);
        AXIS_REGISTER_BUILTIN(NavigationSystem);
        AXIS_REGISTER_BUILTIN(NetworkSystem);
        AXIS_REGISTER_BUILTIN(ParticleSystem);
        AXIS_REGISTER_BUILTIN(PhysicsSystem);
        AXIS_REGISTER_BUILTIN(PlanarReflectionSystem);
        AXIS_REGISTER_BUILTIN(PostProcessSystem);
        AXIS_REGISTER_BUILTIN(ReflectionProbeSystem);
        AXIS_REGISTER_BUILTIN(RenderSystem);
        AXIS_REGISTER_BUILTIN(ScriptableSystem);
        AXIS_REGISTER_BUILTIN(ShadowSystem);
        AXIS_REGISTER_BUILTIN(SkyboxRenderSystem);
        AXIS_REGISTER_BUILTIN(StreamingSystem);
        AXIS_REGISTER_BUILTIN(TerrainSystem);
        AXIS_REGISTER_BUILTIN(TransformSystem);
        AXIS_REGISTER_BUILTIN(TransparentSystem);
        AXIS_REGISTER_BUILTIN(UIInteractSystem);
        AXIS_REGISTER_BUILTIN(UIRenderSystem);
        AXIS_REGISTER_BUILTIN(VideoSystem);
#undef AXIS_REGISTER_BUILTIN
    });
}

std::unique_ptr<IBaseSystem> SystemFactory::Create(const std::string& name)
{
    EnsureBuiltInSystemsRegistered();
    Creator creator;
    {
        std::lock_guard lock(GetMutex());
        const auto it = GetRegistry().find(name);
        if (it == GetRegistry().end())
            return nullptr;
        creator = it->second;
    }
    return creator();
}

std::vector<std::unique_ptr<IBaseSystem>> SystemFactory::CreateAll()
{
    EnsureBuiltInSystemsRegistered();
    std::vector<Creator> creators;
    {
        std::lock_guard lock(GetMutex());
        creators.reserve(GetRegistry().size());
        for (const auto& [name, creator] : GetRegistry()) creators.push_back(creator);
    }

    std::vector<std::unique_ptr<IBaseSystem>> systems;
    systems.reserve(creators.size());
    for (const Creator& creator : creators)
    {
        if (auto sys = creator())
        {
            systems.push_back(std::move(sys));
        }
    }
    return systems;
}


std::vector<std::string> SystemFactory::GetRegisteredNames()
{
    EnsureBuiltInSystemsRegistered();
    std::lock_guard lock(GetMutex());
    std::vector<std::string> names;
    names.reserve(GetRegistry().size());
    for (const auto& [name, creator] : GetRegistry()) names.push_back(name);
    return names;
}
