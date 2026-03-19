#pragma once

#include <core/logic/config_loader.h>
#include <core/logic/runtime_core.h>
#include <platform/interface/i_window.h>
#include <memory>
#include <string>

// Forward declarations
class AnimationSystem;
class AudioService;
class AudioSystem;
class IGraphicsContext;
class InputManager;
class IOHandler;
class IPhysicsWorld;
class KeyboardManager;
class MonitorManager;
class MouseManager;
class ParticleSystem;
class PhysicsSystem;
class PostProcessPipeline;
class RenderSystem;
class ResourceManager;
class Scene;
class SceneManager;
class ScriptableSystem;
class ConfigManager;
class SkyboxRenderSystem;
class State;
class StateMachine;
class SystemManager;
class UIRenderSystem;
class VideoSystem;

// --- Application ---

class Application
{
public:
    Application();
    ~Application();

    bool Initialize(const AppConfig &config);
    void Shutdown();
    void Run();

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        m_RuntimeCore->PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene &GetScene();
    IPhysicsWorld &GetPhysicsWorld();
    IOHandler &GetIOHandler();
    RuntimeCore &GetRuntimeCore();
    StateMachine &GetStateMachine();
    SystemManager &GetSystemManager();
    ResourceManager &GetResourceManager();
    SceneManager &GetSceneManager();
    AudioService &GetAudioService();
    MonitorManager &GetMonitorManager();
    KeyboardManager &GetKeyboard() const;
    MouseManager &GetMouse() const;
    InputManager &GetInputManager() const;
    IGraphicsContext &GetGraphicsContext() const;
    IWindow *GetWindow() const;
    int GetWidth() const;
    int GetHeight() const;

    PostProcessPipeline &GetPostProcess();

    float GetTimeScale() const;
    void SetTimeScale(float timeScale);
    float GetRealDeltaTime() const;
    bool IsPaused() const;
    void SetPaused(bool paused);

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    const AppConfig &GetConfig() const;
    void ApplyConfig(const AppConfig &config);

private:
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<AudioService> m_AudioService;
    std::unique_ptr<SceneManager> m_SceneManager;

    std::unique_ptr<IOHandler> m_IOHandler;
    std::unique_ptr<RuntimeCore> m_RuntimeCore;
    std::unique_ptr<SystemManager> m_SystemManager;
    std::unique_ptr<ConfigManager> m_ConfigManager;
};

// --- App Builder ---

class IAudioEngine;

class AppBuilder
{
public:
    static std::unique_ptr<IGraphicsContext> CreateGraphicsContext(const AppConfig &config);
    static std::unique_ptr<IAudioEngine> CreateAudioEngine(const AppConfig &config);
    static std::unique_ptr<IPhysicsWorld> CreatePhysicsWorld(const AppConfig &config);
    static std::unique_ptr<IWindow> MakeWindow();
};
