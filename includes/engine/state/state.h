#pragma once

#include <scene/scene_manager.h>
#include <resource/resource_manager.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <physic/physic_world.h>

class Application;

class State
{
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) {}
    virtual void OnRender() = 0;
    virtual void OnExit() = 0;

    class RenderSystem &GetRenderSystem();
    class PhysicsSystem &GetPhysicsSystem();
    class AudioSystem &GetAudioSystem();
    class UIRenderSystem &GetUIRenderSystem();
    class UIInteractSystem &GetUIInteractSystem();
    class ScriptableSystem &GetScriptSystem();
    class ParticleSystem &GetParticleSystem();
    class SkyboxRenderSystem &GetSkyboxRenderSystem();
    class AnimationSystem &GetAnimationSystem();
    class VideoSystem &GetVideoSystem();

    class SceneManager &GetSceneManager();
    class ResourceManager &GetResourceManager();
    class SoundManager &GetSoundManager();
    class AppHandler &GetAppHandler();
    class InputManager &GetInputManager();
    class KeyboardManager &GetKeyboard();
    class MouseManager &GetMouse();

    void LoadScene(const std::string &path);
    void UnloadScene(const std::string &path);
    void ChangeScene(const std::string &path);
    void SetCursorMode(CursorMode mode);

    void EnablePhysics(bool enable);
    void EnableRender(bool enable);
    void EnableAudio(bool enable);
    void EnableScript(bool enable);
    void EnableAnimation(bool enable);
    void EnableVideo(bool enable);
    void EnableUIInteract(bool enable);
    void EnableUIRender(bool enable);
    void EnableParticle(bool enable);
    void EnableSkybox(bool enable);

    void EnableLogic(bool enable);

    void SetContext(Application *app) { m_App = app; }

protected:
    Application *m_App = nullptr;
};