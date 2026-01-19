#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <functional>

#include <state/state_machine.h>
#include <ecs/component.h>
#include <physic/physic_world.h>
#include <resource/resource_manager.h>
#include <scene/scene_manager.h>
#include <audio/sound_manager.h>
#include <app/app_handler.h>
#include <app/monitor_manager.h>
#include <app/system_manager.h>
#include <app/engine_loop.h>

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

    // Core accessors
    Scene &GetScene() { return scene; }
    PhysicsWorld &GetPhysicsWorld() { return *physicsWorld; }
    ResourceManager &GetResourceManager() { return *resourceManager; }
    SceneManager &GetSceneManager() { return *sceneManager; }
    SoundManager& GetSoundManager() { return *soundManager; }
    MonitorManager& GetMonitorManager() { return monitorManager; }
    AppHandler& GetAppHandler() const { return *appHandler; }
    StateMachine& GetStateMachine() { return m_StateMachine; }
    SystemManager& GetSystemManager() { return *systemManager; }
    EngineLoop& GetEngineLoop() { return *engineLoop; }
    
    // Input accessors (convenience)
    KeyboardManager &GetKeyboard() const { return appHandler->GetKeyboard(); }
    MouseManager &GetMouse() const { return appHandler->GetMouse(); }
    InputManager &GetInputManager() const { return appHandler->GetInputManager(); }

    // Window accessors
    GLFWwindow *GetWindow() const { return monitorManager.GetWindow(); }
    int GetWidth() const { return monitorManager.GetWidth(); }
    int GetHeight() const { return monitorManager.GetHeight(); }

    // System accessors (delegated to SystemManager)
    RenderSystem &GetRenderSystem() { return systemManager->GetRenderSystem(); }
    UIRenderSystem &GetUIRenderSystem() { return systemManager->GetUIRenderSystem(); }
    SkyboxRenderSystem &GetSkyboxRenderSystem() { return systemManager->GetSkyboxRenderSystem(); }
    UIInteractSystem &GetUIInteractSystem() { return systemManager->GetUIInteractSystem(); }
    PhysicsSystem &GetPhysicsSystem() { return systemManager->GetPhysicsSystem(); }
    AnimationSystem &GetAnimationSystem() { return systemManager->GetAnimationSystem(); }
    ScriptableSystem &GetScriptSystem() { return systemManager->GetScriptSystem(); }
    AudioSystem& GetAudioSystem() { return systemManager->GetAudioSystem(); }
    ParticleSystem& GetParticleSystem() { return systemManager->GetParticleSystem(); }
    VideoSystem& GetVideoSystem() { return systemManager->GetVideoSystem(); }
    PostProcessPipeline& GetPostProcess() { return systemManager->GetPostProcess(); }

    // Configuration methods (delegated to EngineLoop)
    void SetPhysicsStep(float step);
    void SetTimeScale(float scale);
    void SetPaused(bool paused);
    
    float GetTimeScale() const;
    float GetRealDeltaTime() const;
    bool IsPaused() const;

    // Window callbacks
    void OnResize(int width, int height); 
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

private:
    // Core managers (ownership)
    MonitorManager monitorManager;
    std::unique_ptr<AppHandler> appHandler;
    std::unique_ptr<PhysicsWorld> physicsWorld;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<SoundManager> soundManager;
    std::unique_ptr<SceneManager> sceneManager;
    
    // Refactored components
    std::unique_ptr<SystemManager> systemManager;
    std::unique_ptr<EngineLoop> engineLoop;

    Scene scene;
    StateMachine m_StateMachine;
};

