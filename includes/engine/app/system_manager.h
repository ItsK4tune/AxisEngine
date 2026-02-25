#pragma once

#include <ecs/system.h>
#include <graphic/core/post_process_pipeline.h>
#include <memory>
#include <app/config_loader.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_system.h>
#endif

class Scene;
class ResourceManager;
class IPhysicsWorld;
class SoundPlayer;
class Application;
class MouseManager;

enum class SystemGroup : uint32_t
{
    None        = 0,
    Physics     = 1 << 0,
    Script      = 1 << 1,
    Animation   = 1 << 2,
    Audio       = 1 << 3,
    Particle    = 1 << 4,
    Video       = 1 << 5,
    Render      = 1 << 6,
    UI          = 1 << 7,
    Skybox      = 1 << 8,
    
    Logic       = Physics | Script,
    Visuals     = Animation | Video | Audio | Particle,
    Graphics    = Render | UI | Skybox | Particle,
    All         = 0xFFFFFFFF
};

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void InitializeSystems(ResourceManager &res, int width, int height, Application* app);
    void ApplyConfig(const AppConfig &config);
    void ShutdownSystems();

    void FixedUpdateSystems(Scene &scene, IPhysicsWorld &phys, float fixedDt, uint32_t mask = 0xFFFFFFFF);

    void UpdateLogic(Scene &scene, float deltaTime, float realDeltaTime, Application* app, MouseManager &mouse, uint32_t mask = 0xFFFFFFFF);
    void UpdateVisuals(Scene &scene, float deltaTime, ResourceManager &res, SoundPlayer &sound, uint32_t mask = 0xFFFFFFFF);

    void RenderShadows(Scene &scene, uint32_t mask = 0xFFFFFFFF);
    void RenderSystems(Scene &scene, ResourceManager &res, int width, int height, uint32_t mask = 0xFFFFFFFF);

#ifdef ENABLE_DEBUG_SYSTEM
    void UpdateDebugSystem(float realDeltaTime);
    void RenderDebugSystem(Scene &scene);
    DebugSystem *GetDebugSystem() { return debugSystem.get(); }
#endif

    RenderSystem &GetRenderSystem() { return renderSystem; }
    UIRenderSystem &GetUIRenderSystem() { return uiRenderSystem; }
    SkyboxRenderSystem &GetSkyboxRenderSystem() { return skyboxRenderSystem; }
    PhysicsSystem &GetPhysicsSystem() { return physicsSystem; }
    AnimationSystem &GetAnimationSystem() { return animationSystem; }
    ScriptableSystem &GetScriptSystem() { return scriptSystem; }
    AudioSystem &GetAudioSystem() { return audioSystem; }
    ParticleSystem &GetParticleSystem() { return particleSystem; }
    VideoSystem &GetVideoSystem() { return videoSystem; }
    PostProcessPipeline &GetPostProcess() { return postProcess; }

private:
    PhysicsSystem physicsSystem;
    RenderSystem renderSystem;
    AnimationSystem animationSystem;
    UIRenderSystem uiRenderSystem;
    ScriptableSystem scriptSystem;
    SkyboxRenderSystem skyboxRenderSystem;
    AudioSystem audioSystem;
    ParticleSystem particleSystem;
    VideoSystem videoSystem;

    PostProcessPipeline postProcess;

#ifdef ENABLE_DEBUG_SYSTEM
    std::unique_ptr<DebugSystem> debugSystem;
#endif
};
