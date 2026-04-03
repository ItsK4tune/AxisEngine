#pragma once

#include <core/logic/service_locator.h>
#include <core/logic/logger.h>
#include <platform/interface/cursor_mode.h>
#include <string>
#include <vector>

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

class EngineAccessor {
public:
    virtual ~EngineAccessor() = default;

    template <typename T>
    T& Get() const { return ServiceLocator::Instance().Require<T>(); }

    template <typename T>
    T& GetSystem() const { 
        auto* sys = Get<SystemManager>().template GetSystem<T>();
        if (!sys) throw std::runtime_error("System not found: " + std::string(typeid(T).name()));
        return *sys;
    }

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

    // Generic system enable/disable — no concrete type dependency
    void EnableSystem(const std::string& systemName, bool enable);

    // Backward-compatible convenience wrappers (delegate to generic EnableSystem)
    void EnablePhysics(bool enable)    { EnableSystem("PhysicsSystem", enable); }
    void EnableRender(bool enable)     { EnableSystem("RenderSystem", enable); }
    void EnableAudio(bool enable)      { EnableSystem("AudioSystem", enable); }
    void EnableScript(bool enable)     { EnableSystem("ScriptableSystem", enable); }
    void EnableAnimation(bool enable)  { EnableSystem("AnimationSystem", enable); }
    void EnableVideo(bool enable)      { EnableSystem("VideoSystem", enable); }
    void EnableUIRender(bool enable)   { EnableSystem("UIRenderSystem", enable); }
    void EnableParticle(bool enable)   { EnableSystem("ParticleSystem", enable); }
    void EnableSkybox(bool enable)     { EnableSystem("SkyboxRenderSystem", enable); }
    void EnableNavigation(bool enable) { EnableSystem("NavigationSystem", enable); }
    void EnableLogic(bool enable);

    bool GetAction(const std::string &name) const;
    bool GetActionDown(const std::string &name) const;
    bool GetActionUp(const std::string &name) const;

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    const AppConfig& GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    class RenderSystem& GetRenderSystem() const;
    class PhysicsSystem& GetPhysicsSystem() const;
    class AudioSystem& GetAudioSystem() const;
    class UIRenderSystem& GetUIRenderSystem() const;
    class ScriptableSystem& GetScriptSystem() const;
    class ParticleSystem& GetParticleSystem() const;
    class SkyboxRenderSystem& GetSkyboxRenderSystem() const;
    class AnimationSystem& GetAnimationSystem() const;
    class VideoSystem& GetVideoSystem() const;
    class NavigationSystem& GetNavigationSystem() const;

    Scene& GetScene() const;
    class SceneManager& GetSceneManager() const;
    class ResourceManager& GetResourceManager() const;
    class AudioService& GetAudioService() const;
    class IOHandler& GetIOHandler() const;
    class InputManager& GetInputManager() const;
    class KeyboardManager& GetKeyboard() const;
    class MouseManager& GetMouse() const;
    class RuntimeCore& GetRuntimeCore() const;
    class SystemManager& GetSystemManager() const;

    void SetActiveScene(Scene* scene) { m_ActiveScene = scene; }

protected:
    Scene* m_ActiveScene = nullptr;
};
