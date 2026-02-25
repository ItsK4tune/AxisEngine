#pragma once

#include <memory>
#include <string>
#include <app/config_loader.h>
#include <app/runtime_core.h>

class IOHandler;
class ContentService;
class SystemManager;
class ResourceManager;
class SceneManager;
class SoundPlayer;
class Scene;
class IPhysicsWorld;
class IWindow;
class IGraphicsContext;
class MonitorManager;
class KeyboardManager;
class MouseManager;
class InputManager;
class StateMachine;
class RenderSystem;
class PhysicsSystem;
class AudioSystem;
class UIRenderSystem;
class ScriptableSystem;
class ParticleSystem;
class SkyboxRenderSystem;
class AnimationSystem;
class VideoSystem;
class PostProcessPipeline;
class State;

struct WorldContext;
struct IOContext;
struct SystemContext;

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

    Scene&           GetScene();
    IPhysicsWorld&   GetPhysicsWorld();
    IOHandler&       GetIOHandler();
    ContentService&  GetContentService();
    RuntimeCore&     GetRuntimeCore();
    StateMachine&    GetStateMachine();
    SystemManager&   GetSystemManager();
    ResourceManager& GetResourceManager();
    SceneManager&    GetSceneManager();
    SoundPlayer&     GetSoundPlayer();
    MonitorManager&  GetMonitorManager();
    KeyboardManager& GetKeyboard() const;
    MouseManager&    GetMouse() const;
    InputManager&    GetInputManager() const;
    IGraphicsContext& GetGraphicsContext() const;
    IWindow*         GetWindow() const;
    int              GetWidth() const;
    int              GetHeight() const;

    RenderSystem&        GetRenderSystem();
    PhysicsSystem&       GetPhysicsSystem();
    AudioSystem&         GetAudioSystem();
    UIRenderSystem&      GetUIRenderSystem();
    ScriptableSystem&    GetScriptSystem();
    ParticleSystem&      GetParticleSystem();
    SkyboxRenderSystem&  GetSkyboxRenderSystem();
    AnimationSystem&     GetAnimationSystem();
    VideoSystem&         GetVideoSystem();
    PostProcessPipeline& GetPostProcess();

    WorldContext  GetWorldContext();
    IOContext     GetIOContext();
    SystemContext GetSystemContext();

    float GetTimeScale() const;
    void  SetTimeScale(float timeScale);
    float GetRealDeltaTime() const;
    bool  IsPaused() const;
    void  SetPaused(bool paused);

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    const AppConfig& GetConfig() const;
    void ApplyConfig(const AppConfig& config);

private:
    // Scene stored as unique_ptr so Scene type can be forward-declared in this header
    std::unique_ptr<Scene>           m_Scene;
    std::unique_ptr<IPhysicsWorld>   m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<SoundPlayer>     m_SoundPlayer;
    std::unique_ptr<SceneManager>    m_SceneManager;

    std::unique_ptr<IOHandler>       m_IOHandler;
    std::unique_ptr<ContentService>  m_ContentService;
    std::unique_ptr<RuntimeCore>     m_RuntimeCore;
    std::unique_ptr<SystemManager>   m_SystemManager;

    AppConfig m_Config;
};
