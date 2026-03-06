#pragma once
#include <core/engine_context.h>
#include <string>
#include <vector>

namespace Input { enum class CursorMode; }
struct SceneRecord;
struct AppConfig;

class EngineAccessor {
public:
    virtual ~EngineAccessor() = default;

    class RenderSystem&       GetRenderSystem() const;
    class PhysicsSystem&      GetPhysicsSystem() const;
    class AudioSystem&        GetAudioSystem() const;
    class UIRenderSystem&     GetUIRenderSystem() const;
    class ScriptableSystem&   GetScriptSystem() const;
    class ParticleSystem&     GetParticleSystem() const;
    class SkyboxRenderSystem& GetSkyboxRenderSystem() const;
    class AnimationSystem&    GetAnimationSystem() const;
    class VideoSystem&        GetVideoSystem() const;

    class Scene&           GetScene() const;
    class SceneManager&    GetSceneManager() const;
    class ResourceManager& GetResourceManager() const;
    class SoundPlayer&     GetSoundPlayer() const;
    class IOHandler&       GetIOHandler() const;
    class InputManager&    GetInputManager() const;
    class KeyboardManager& GetKeyboard() const;
    class MouseManager&    GetMouse() const;

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
    void SetCursorMode(Input::CursorMode mode);

    void EnablePhysics(bool enable);
    void EnableRender(bool enable);
    void EnableAudio(bool enable);
    void EnableScript(bool enable);
    void EnableAnimation(bool enable);
    void EnableVideo(bool enable);
    void EnableUIRender(bool enable);
    void EnableParticle(bool enable);
    void EnableSkybox(bool enable);
    void EnableLogic(bool enable);

    bool GetAction(const std::string &name) const;
    bool GetActionDown(const std::string &name) const;
    bool GetActionUp(const std::string &name) const;

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    const AppConfig& GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    void SetContext(EngineContext ctx) { m_Ctx = ctx; }

protected:
    EngineContext m_Ctx;
};
