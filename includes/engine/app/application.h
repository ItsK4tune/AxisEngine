#pragma once

#include <memory>
#include <string>
#include <interface/physics/i_physics_world.h>

#include <app/io_handler.h>
#include <app/content_service.h>
#include <app/runtime_core.h>
#include <app/system_manager.h>
#include <scene/scene.h>

#include <resource/resource_manager.h>
#include <scene/scene_manager.h>
#include <audio/sound_player.h>

#include <interface/window/i_window.h>
#include <app/config_loader.h>

class IGraphicsContext;
class State;

class Application
{
public:
    Application();
    ~Application();

    bool Init(const AppConfig& config);
    void Run();

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        m_RuntimeCore->PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene& GetScene() { return m_Scene; }
    IPhysicsWorld& GetPhysicsWorld() { return *m_PhysicsWorld; }
    
    IOHandler& GetIOHandler() { return *m_IOHandler; }
    ContentService& GetContentService() { return *m_ContentService; }
    RuntimeCore& GetRuntimeCore() { return *m_RuntimeCore; }
    StateMachine& GetStateMachine() { return m_RuntimeCore->GetStateMachine(); }
    SystemManager& GetSystemManager() { return *m_SystemManager; }

    ResourceManager& GetResourceManager() { return *m_ResourceManager; }
    SceneManager& GetSceneManager() { return *m_SceneManager; }
    SoundPlayer& GetSoundPlayer() { return *m_SoundPlayer; }
    MonitorManager& GetMonitorManager() { return m_IOHandler->GetMonitorManager(); }
    KeyboardManager& GetKeyboard() const { return m_IOHandler->GetKeyboard(); }
    MouseManager& GetMouse() const { return m_IOHandler->GetMouse(); }
    InputManager& GetInputManager() const { return m_IOHandler->GetInputManager(); }
    IGraphicsContext& GetGraphicsContext() const { return m_IOHandler->GetGraphicsContext(); }
    
    IWindow* GetWindow() const { return m_IOHandler->GetMonitorManager().GetWindow(); }
    int GetWidth() const { return m_IOHandler->GetMonitorManager().GetWidth(); }
    int GetHeight() const { return m_IOHandler->GetMonitorManager().GetHeight(); }

    class RenderSystem& GetRenderSystem();
    class PhysicsSystem& GetPhysicsSystem();
    class AudioSystem& GetAudioSystem();
    class UIRenderSystem& GetUIRenderSystem();
    class UIInteractSystem& GetUIInteractSystem();
    class ScriptableSystem& GetScriptSystem();
    class ParticleSystem& GetParticleSystem();
    class SkyboxRenderSystem& GetSkyboxRenderSystem();
    class AnimationSystem& GetAnimationSystem();
    class VideoSystem& GetVideoSystem();
    class PostProcessPipeline& GetPostProcess();

    float GetTimeScale() const;
    void SetTimeScale(float timeScale);
    float GetRealDeltaTime() const;
    bool IsPaused() const;
    void SetPaused(bool paused);

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

private:
    Scene m_Scene;
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<SoundPlayer> m_SoundPlayer;
    std::unique_ptr<SceneManager> m_SceneManager;
    
    std::unique_ptr<IOHandler> m_IOHandler;
    std::unique_ptr<ContentService> m_ContentService;
    std::unique_ptr<RuntimeCore> m_RuntimeCore;
    std::unique_ptr<SystemManager> m_SystemManager;
    
    AppConfig m_Config;
};
