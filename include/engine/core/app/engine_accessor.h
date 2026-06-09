#pragma once

#include <core/logic/localization_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <platform/interface/cursor_mode.h>
#include <ecs/logic/system_manager.h>
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

class EngineAccessor
{
public:
    virtual ~EngineAccessor() = default;

    template <typename T>
    T& Get() const
    {
        return ServiceLocator::Instance().Require<T>();
    }

    template <typename T>
    T* Resolve() const
    {
        return ServiceLocator::Instance().Resolve<T>();
    }

    template <typename T>
    bool Has() const
    {
        return ServiceLocator::Instance().Has<T>();
    }

    template <typename T>
    T& GetSystem() const
    {
        auto* sys = Get<SystemManager>().GetSystem<T>();
        if (!sys)
            throw std::runtime_error("System not found: " + std::string(typeid(T).name()));
        return *sys;
    }

    Scene& GetScene() const;
    void LoadScene(const std::string& path, bool persistent = false);
    void QueueLoadScene(const std::string& path, bool persistent = false);
    void UnloadScene(const std::string& path);
    void UnloadScene(const SceneRecord* rec);
    void ChangeScene(const std::string& path);
    void PopScene();
    void QueuePopScene();
    bool IsSceneLoaded(const std::string& path);
    void LogAllScenes();
    std::vector<const SceneRecord*> GetScenes();

    bool LoadInputBindings(const std::string& path);
    bool SaveInputBindings(const std::string& path);
    bool LoadDataNodes(const std::string& path);
    bool SaveDataNodes(const std::string& path);
    bool GetAction(const std::string& name) const;
    bool GetActionDown(const std::string& name) const;
    bool GetActionUp(const std::string& name) const;
    void SetCursorMode(CursorMode mode);

    void LoadLanguage(const std::string& path, const std::string& name = "");
    void SetLanguage(const std::string& name);
    std::string GetLanguage() const;
    std::string GetTranslation(const std::string& key) const;

    template <typename... Args>
    std::string GetTranslation(const std::string& key, Args... args) const
    {
        if (auto* loc = Resolve<LocalizationSystem>())
        {
            return loc->GetFormat(key, args...);
        }
        return "[MISSING: " + key + "]";
    }

    void EnableSystem(const std::string& systemName, bool enable);
    void EnablePhysics(bool enable)
    {
        EnableSystem("PhysicsSystem", enable);
    }
    void EnableRender(bool enable)
    {
        EnableSystem("RenderSystem", enable);
    }
    void EnableAudio(bool enable)
    {
        EnableSystem("AudioSystem", enable);
    }
    void EnableScript(bool enable)
    {
        EnableSystem("ScriptableSystem", enable);
    }
    void EnableAnimation(bool enable)
    {
        EnableSystem("AnimationSystem", enable);
    }
    void EnableVideo(bool enable)
    {
        EnableSystem("VideoSystem", enable);
    }
    void EnableUIRender(bool enable)
    {
        EnableSystem("UIRenderSystem", enable);
    }
    void EnableParticle(bool enable)
    {
        EnableSystem("ParticleSystem", enable);
    }
    void EnableSkybox(bool enable)
    {
        EnableSystem("SkyboxRenderSystem", enable);
    }
    void EnableNavigation(bool enable)
    {
        EnableSystem("NavigationSystem", enable);
    }
    void EnableLogic(bool enable);

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    AppConfig GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    void SetActiveScene(Scene* scene)
    {
        m_ActiveScene = scene;
    }

protected:
    Scene* m_ActiveScene = nullptr;
};
