#pragma once

#include <core/scene_manager.h>
#include <core/resource_manager.h>
#include <core/keyboard_manager.h>
#include <core/mouse_manager.h>
#include <physic/physic_world.h>

class Application;

class State {
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) {}
    virtual void OnRender() = 0;
    virtual void OnExit() = 0;

    // System Accessors
    class RenderSystem& GetRenderSystem();
    class PhysicsSystem& GetPhysicsSystem();
    class AudioSystem& GetAudioSystem();
    class UIRenderSystem& GetUIRenderSystem();
    class UIInteractSystem& GetUIInteractSystem();
    class ScriptableSystem& GetScriptSystem();
    class ParticleSystem& GetParticleSystem();
    class SkyboxRenderSystem& GetSkyboxRenderSystem();
    class AnimationSystem& GetAnimationSystem();

    // Manager Accessors
    class SceneManager& GetSceneManager();
    class ResourceManager& GetResourceManager();
    class SoundManager& GetSoundManager();
    class AppHandler& GetAppHandler();
    class InputManager& GetInputManager();
    class KeyboardManager& GetKeyboard();
    class MouseManager& GetMouse();

protected:
    Application* m_App = nullptr;
    void SetContext(Application* app) { m_App = app; }
    friend class StateMachine;
};