#pragma once

#include <ecs/system.h>
#include <graphic/post_process_pipeline.h>
#include <memory>

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_system.h>
#endif

// Forward declarations
class Scene;
class ResourceManager;
class PhysicsWorld;
class SoundManager;
class Application;
class MouseManager;

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void InitializeSystems(ResourceManager& res, int width, int height, Application* app);
    void ShutdownSystems();

    // Update all systems
    void FixedUpdateSystems(Scene& scene, PhysicsWorld& phys, float fixedDt);
    void UpdateSystems(Scene& scene, float deltaTime, float realDeltaTime, 
                      Application* app, ResourceManager& res, 
                      SoundManager& sound, MouseManager& mouse);
    
    // Render all systems
    void RenderShadows(Scene& scene);
    void RenderSystems(Scene& scene, ResourceManager& res, int width, int height);

#ifdef ENABLE_DEBUG_SYSTEM
    void UpdateDebugSystem(float realDeltaTime);
    void RenderDebugSystem(Scene& scene);
    DebugSystem* GetDebugSystem() { return debugSystem.get(); }
#endif

    // System accessors
    RenderSystem& GetRenderSystem() { return renderSystem; }
    UIRenderSystem& GetUIRenderSystem() { return uiRenderSystem; }
    SkyboxRenderSystem& GetSkyboxRenderSystem() { return skyboxRenderSystem; }
    UIInteractSystem& GetUIInteractSystem() { return uiInteractSystem; }
    PhysicsSystem& GetPhysicsSystem() { return physicsSystem; }
    AnimationSystem& GetAnimationSystem() { return animationSystem; }
    ScriptableSystem& GetScriptSystem() { return scriptSystem; }
    AudioSystem& GetAudioSystem() { return audioSystem; }
    ParticleSystem& GetParticleSystem() { return particleSystem; }
    VideoSystem& GetVideoSystem() { return videoSystem; }
    PostProcessPipeline& GetPostProcess() { return postProcess; }

private:
    // ECS Systems
    PhysicsSystem physicsSystem;
    RenderSystem renderSystem;
    AnimationSystem animationSystem;
    UIInteractSystem uiInteractSystem;
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

