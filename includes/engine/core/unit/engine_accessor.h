#pragma once

#include <core/unit/engine_context.h>
#include <platform/interface/cursor_mode.h>
#include <string>
#include <vector>

// Forward declarations for systems and managers
class RenderSystem;
class PhysicsSystem;
class AudioSystem;
class UIRenderSystem;
class ScriptableSystem;
class ParticleSystem;
class SkyboxRenderSystem;
class AnimationSystem;
class VideoSystem;

class Scene;
class SceneManager;
class ResourceManager;
class SoundPlayer;
class IOHandler;
class InputManager;
class KeyboardManager;
class MouseManager;
struct SceneRecord;
struct AppConfig;

// --- Engine Accessor ---

class EngineAccessor {
public:
    virtual ~EngineAccessor() = default;

    RenderSystem&       GetRenderSystem() const;
    PhysicsSystem&      GetPhysicsSystem() const;
    AudioSystem&        GetAudioSystem() const;
    UIRenderSystem&     GetUIRenderSystem() const;
    ScriptableSystem&   GetScriptSystem() const;
    ParticleSystem&     GetParticleSystem() const;
    SkyboxRenderSystem& GetSkyboxRenderSystem() const;
    AnimationSystem&    GetAnimationSystem() const;
    VideoSystem&        GetVideoSystem() const;

    Scene&           GetScene() const;
    SceneManager&    GetSceneManager() const;
    ResourceManager& GetResourceManager() const;
    SoundPlayer&     GetSoundPlayer() const;
    IOHandler&       GetIOHandler() const;
    InputManager&    GetInputManager() const;
    KeyboardManager& GetKeyboard() const;
    MouseManager&    GetMouse() const;

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
