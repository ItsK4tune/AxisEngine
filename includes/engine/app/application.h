#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <functional>

#include <state/state_machine.h>
#include <ecs/component.h>
#include <ecs/system.h>
#include <physic/physic_world.h>
#include <resource/resource_manager.h>
#include <scene/scene_manager.h>
#include <audio/sound_manager.h>
#include <graphic/post_process_pipeline.h>
#include <app/app_handler.h>
#include <app/monitor_manager.h>
#include <debug/debug_system.h>

class Application
{
public:
    Application();
    ~Application();

    bool Init();
    void Run();

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        m_StateMachine.PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene &GetScene() { return scene; }
    PhysicsWorld &GetPhysicsWorld() { return *physicsWorld; }

    ResourceManager &GetResourceManager() { return *resourceManager; }
    SceneManager &GetSceneManager() { return *sceneManager; }
    MonitorManager& GetMonitorManager() { return monitorManager; }
    AppHandler& GetAppHandler() const { return *appHandler; }
    
    KeyboardManager &GetKeyboard() const { return appHandler->GetKeyboard(); }
    MouseManager &GetMouse() const { return appHandler->GetMouse(); }
    InputManager &GetInputManager() const { return appHandler->GetInputManager(); }

    GLFWwindow *GetWindow() const { return monitorManager.GetWindow(); }
    int GetWidth() const { return monitorManager.GetWidth(); }
    int GetHeight() const { return monitorManager.GetHeight(); }

    RenderSystem &GetRenderSystem() { return renderSystem; }
    UIRenderSystem &GetUIRenderSystem() { return uiRenderSystem; }
    SkyboxRenderSystem &GetSkyboxRenderSystem() { return skyboxRenderSystem; }
    UIInteractSystem &GetUIInteractSystem() { return uiInteractSystem; }
    PhysicsSystem &GetPhysicsSystem() { return physicsSystem; }
    AnimationSystem &GetAnimationSystem() { return animationSystem; }
    ScriptableSystem &GetScriptSystem() { return scriptSystem; }
    SoundManager& GetSoundManager() { return *soundManager; }

    PostProcessPipeline& GetPostProcess() { return postProcess; }
    AudioSystem& GetAudioSystem() { return audioSystem; }
    ParticleSystem& GetParticleSystem() { return particleSystem; }
    VideoSystem& GetVideoSystem() { return videoSystem; }

    // Deprecated/Delegated Configuration Methods
    void SetPhysicsStep(float step);

    void OnResize(int width, int height); 
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

private:
    MonitorManager monitorManager;
    std::unique_ptr<AppHandler> appHandler;

    StateMachine m_StateMachine;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float m_Accumulator = 0.0f;
    float m_FixedDeltaTime = 1.0f / 60.0f;

    std::unique_ptr<PhysicsWorld> physicsWorld;
    Scene scene;

    std::unique_ptr<SoundManager> soundManager;
    std::unique_ptr<SceneManager> sceneManager;
    std::unique_ptr<ResourceManager> resourceManager;

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