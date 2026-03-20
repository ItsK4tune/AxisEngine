#pragma once

#include <core/logic/service_locator.h>
#include <core/app/system_manager.h>
#include <platform/interface/cursor_mode.h>
#include <string>
#include <vector>

// Forward declarations for systems and managers
class Scene;
class SceneManager;
class ResourceManager;
class AudioService;
class IOHandler;
class InputManager;
class KeyboardManager;
class MouseManager;
class RuntimeCore;
class SystemManager;
struct SceneRecord;
struct AppConfig;

// --- Engine Accessor ---

class EngineAccessor {
public:
    virtual ~EngineAccessor() = default;

    // Template-based accessors (Modern approach)
    template <typename T>
    T& Get() const { return ServiceLocator::Instance().Require<T>(); }

    template <typename T>
    T& GetSystem() const { return *ServiceLocator::Instance().Require<SystemManager>().GetSystem<T>(); }

    // Legacy/Convenience getters (Systems)
    class RenderSystem&       GetRenderSystem() const;
    class PhysicsSystem&      GetPhysicsSystem() const;
    class AudioSystem&        GetAudioSystem() const;
    class UIRenderSystem&     GetUIRenderSystem() const;
    class ScriptableSystem&   GetScriptSystem() const;
    class ParticleSystem&     GetParticleSystem() const;
    class SkyboxRenderSystem& GetSkyboxRenderSystem() const;
    class AnimationSystem&    GetAnimationSystem() const;
    class VideoSystem&        GetVideoSystem() const;
    class NavigationSystem&   GetNavigationSystem() const;



    // Legacy/Convenience getters
    Scene&           GetScene() const;
    SceneManager&    GetSceneManager() const;
    ResourceManager& GetResourceManager() const;
    AudioService&    GetAudioService() const;
    IOHandler&       GetIOHandler() const;
    InputManager&    GetInputManager() const;
    KeyboardManager& GetKeyboard() const;
    MouseManager&    GetMouse() const;
    RuntimeCore&     GetRuntimeCore() const;
    SystemManager&   GetSystemManager() const;


    void LoadScene(const std::string& path, bool persistent = false);
    void LoadInputBindings(const std::string& path);
    void QueueLoadScene(const std::string& path, bool persistent = false);
    void UnloadScene(const std::string& path);
    void UnloadScene(const SceneRecord* rec);
    void ChangeScene(const std::string& path);
    void PopScene();
    void QueuePopScene();
    bool IsSceneLoaded(const std::string& path);
    void LogAllScenes();
    std::vector<const SceneRecord*> GetScenes();
    void SetCursorMode(CursorMode mode);

    void EnablePhysics(bool enable);
    void EnableRender(bool enable);
    void EnableAudio(bool enable);
    void EnableScript(bool enable);
    void EnableAnimation(bool enable);
    void EnableVideo(bool enable);
    void EnableUIRender(bool enable);
    void EnableParticle(bool enable);
    void EnableSkybox(bool enable);
    void EnableNavigation(bool enable);
    void EnableLogic(bool enable);

    bool GetAction(const std::string &name) const;
    bool GetActionDown(const std::string &name) const;
    bool GetActionUp(const std::string &name) const;

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    const AppConfig& GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    void SetActiveScene(Scene* scene) { m_ActiveScene = scene; }

protected:
    Scene* m_ActiveScene = nullptr;
};
