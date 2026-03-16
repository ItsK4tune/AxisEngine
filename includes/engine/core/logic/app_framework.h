#pragma once

#include <core/logic/config_loader.h>
#include <core/logic/engine_core.h>
#include <core/unit/engine_context.h>
#include <platform/interface/i_window.h>
#include <memory>
#include <string>

// Forward declarations
class AnimationSystem;
class AudioSystem;
class ContentService;
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
class SkyboxRenderSystem;
class SoundPlayer;
class State;
class StateMachine;
class SystemManager;
class UIRenderSystem;
class VideoSystem;
struct IOContext;
struct SystemContext;

// --- Application ---

class Application
{
public:
    Application();
    ~Application();

    bool Initialize(const AppConfig &config);
    void Shutdown();
    EngineContext GetContext();
    void Run();

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        m_RuntimeCore->PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene &GetScene();
    IPhysicsWorld &GetPhysicsWorld();
    IOHandler &GetIOHandler();
    ContentService &GetContentService();
    RuntimeCore &GetRuntimeCore();
    StateMachine &GetStateMachine();
    SystemManager &GetSystemManager();
    ResourceManager &GetResourceManager();
    SceneManager &GetSceneManager();
    SoundPlayer &GetSoundPlayer();
    MonitorManager &GetMonitorManager();
    KeyboardManager &GetKeyboard() const;
    MouseManager &GetMouse() const;
    InputManager &GetInputManager() const;
    IGraphicsContext &GetGraphicsContext() const;
    IWindow *GetWindow() const;
    int GetWidth() const;
    int GetHeight() const;

    [[deprecated("Use GetContext().GetSystem<RenderSystem>() instead")]]
    RenderSystem &GetRenderSystem();
    [[deprecated("Use GetContext().GetSystem<PhysicsSystem>() instead")]]
    PhysicsSystem &GetPhysicsSystem();
    [[deprecated("Use GetContext().GetSystem<AudioSystem>() instead")]]
    AudioSystem &GetAudioSystem();
    [[deprecated("Use GetContext().GetSystem<UIRenderSystem>() instead")]]
    UIRenderSystem &GetUIRenderSystem();
    [[deprecated("Use GetContext().GetSystem<ScriptableSystem>() instead")]]
    ScriptableSystem &GetScriptSystem();
    [[deprecated("Use GetContext().GetSystem<ParticleSystem>() instead")]]
    ParticleSystem &GetParticleSystem();
    [[deprecated("Use GetContext().GetSystem<SkyboxRenderSystem>() instead")]]
    SkyboxRenderSystem &GetSkyboxRenderSystem();
    [[deprecated("Use GetContext().GetSystem<AnimationSystem>() instead")]]
    AnimationSystem &GetAnimationSystem();
    [[deprecated("Use GetContext().GetSystem<VideoSystem>() instead")]]
    VideoSystem &GetVideoSystem();
    PostProcessPipeline &GetPostProcess();

    IOContext GetIOContext();
    SystemContext GetSystemContext();

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
    std::unique_ptr<SoundPlayer> m_SoundPlayer;
    std::unique_ptr<SceneManager> m_SceneManager;

    std::unique_ptr<IOHandler> m_IOHandler;
    std::unique_ptr<ContentService> m_ContentService;
    std::unique_ptr<RuntimeCore> m_RuntimeCore;
    std::unique_ptr<SystemManager> m_SystemManager;

    AppConfig m_Config;
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

// --- App Handler ---

class AppHandler
{
public:
    AppHandler(IWindow* window);
    ~AppHandler();

    void ProcessInput(IWindow* window);

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    KeyboardManager& GetKeyboard() const { return *m_KeyboardManager; }
    MouseManager& GetMouse() const { return *m_MouseManager; }
    InputManager& GetInputManager() const { return *m_InputManager; }

private:
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;
};
