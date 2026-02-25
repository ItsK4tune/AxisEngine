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

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void InitializeSystems(ResourceManager &res, int width, int height, Application* app);
    void ApplyConfig(const AppConfig &config);
    void ShutdownSystems();

    void FixedUpdateSystems(Scene &scene, IPhysicsWorld &phys, float fixedDt);

    void UpdateLogic(Scene &scene, float deltaTime, float realDeltaTime, Application* app, MouseManager &mouse);
    void UpdateVisuals(Scene &scene, float deltaTime, ResourceManager &res, SoundPlayer &sound);

    void RenderShadows(Scene &scene);
    void RenderSystems(Scene &scene, ResourceManager &res, int width, int height);

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
