#pragma once

#include <core/logic/service_locator.h>
#include <ecs/logic/system_manager.h>
#include <core/logic/logger.h>
#include <platform/interface/cursor_mode.h>
#include <string>
#include <vector>

// Forward declarations for systems and managers
struct Scene;
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
    T& GetSystem() const { 
        T* sys = ServiceLocator::Instance().Require<SystemManager>().GetSystem<T>();
        if (!sys) {
            // We crash with a controlled message rather than access violation if possible, 
            // but in many cases we just want to know WHICH system is missing.
            LOGGER_ERROR("EngineAccessor") << "System not found or not registered: " << typeid(T).name();
            // Since we must return a reference, we still have to handle the failure.
            // For now, let's keep the dereference but the log will help pinpoint it.
        }
        return *sys; 
    }

    // Legacy/Convenience getters (Systems) - DEPRECATED: Use GetSystem<T>() instead.
    [[deprecated("Use GetSystem<RenderSystem>() instead")]]
    class RenderSystem&       GetRenderSystem() const;
    [[deprecated("Use GetSystem<PhysicsSystem>() instead")]]
    class PhysicsSystem&      GetPhysicsSystem() const;
    [[deprecated("Use GetSystem<AudioSystem>() instead")]]
    class AudioSystem&        GetAudioSystem() const;
    [[deprecated("Use GetSystem<UIRenderSystem>() instead")]]
    class UIRenderSystem&     GetUIRenderSystem() const;
    [[deprecated("Use GetSystem<ScriptableSystem>() instead")]]
    class ScriptableSystem&   GetScriptSystem() const;
    [[deprecated("Use GetSystem<ParticleSystem>() instead")]]
    class ParticleSystem&     GetParticleSystem() const;
    [[deprecated("Use GetSystem<SkyboxRenderSystem>() instead")]]
    class SkyboxRenderSystem& GetSkyboxRenderSystem() const;
    [[deprecated("Use GetSystem<AnimationSystem>() instead")]]
    class AnimationSystem&    GetAnimationSystem() const;
    [[deprecated("Use GetSystem<VideoSystem>() instead")]]
    class VideoSystem&        GetVideoSystem() const;
    [[deprecated("Use GetSystem<NavigationSystem>() instead")]]
    class NavigationSystem&   GetNavigationSystem() const;

    // Legacy/Convenience getters - DEPRECATED: Use Get<T>() instead.
    [[deprecated("Use Get<Scene>() instead")]]
    Scene&           GetScene() const;
    [[deprecated("Use Get<SceneManager>() instead")]]
    SceneManager&    GetSceneManager() const;
    [[deprecated("Use Get<ResourceManager>() instead")]]
    ResourceManager& GetResourceManager() const;
    [[deprecated("Use Get<AudioService>() instead")]]
    AudioService&    GetAudioService() const;
    [[deprecated("Use Get<IOHandler>() instead")]]
    IOHandler&       GetIOHandler() const;
    [[deprecated("Use Get<InputManager>() instead")]]
    InputManager&    GetInputManager() const;
    [[deprecated("Use Get<KeyboardManager>() instead")]]
    KeyboardManager& GetKeyboard() const;
    [[deprecated("Use Get<MouseManager>() instead")]]
    MouseManager&    GetMouse() const;
    [[deprecated("Use Get<RuntimeCore>() instead")]]
    RuntimeCore&     GetRuntimeCore() const;
    [[deprecated("Use Get<SystemManager>() instead")]]
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
