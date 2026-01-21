#pragma once

#include <scene/scene.h>
#include <ecs/component.h>
#include <vector>
#include <functional>

class Application;

class Scriptable
{
public:
    virtual ~Scriptable() {}

    void Init(entt::entity entity, Scene *scene, Application *app)
    {
        m_Entity = entity;
        m_Scene = scene;
        m_App = app;
    }

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}

    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnReset() {}

    void SetEnabled(bool enabled)
    {
        if (m_Enabled == enabled)
            return;
        m_Enabled = enabled;
        if (m_Enabled)
            OnEnable();
        else
            OnDisable();
    }

    bool IsEnabled() const { return m_Enabled; }

    void SetRunWhenPaused(bool run) { m_RunWhenPaused = run; }
    bool CanRunWhenPaused() const { return m_RunWhenPaused; }

    virtual void OnLeftClick() {}
    virtual void OnLeftHold(float duration) {}
    virtual void OnLeftRelease(float duration) {}

    virtual void OnRightClick() {}
    virtual void OnRightHold(float duration) {}
    virtual void OnRightRelease(float duration) {}

    virtual void OnMiddleClick() {}
    virtual void OnMiddleHold(float duration) {}
    virtual void OnMiddleRelease(float duration) {}

    virtual void OnHoverEnter() {}
    virtual void OnHoverStay() {}
    virtual void OnHoverExit() {}

    enum class InputEvent
    {
        Pressed,
        Held,
        Released
    };
    struct KeyBinding
    {
        int key;
        InputEvent event;
        std::function<void()> callback;
    };

    void BindKey(int key, InputEvent event, std::function<void()> callback)
    {
        m_KeyBindings.push_back({key, event, callback});
    }

    bool IsHovered() const { return m_IsHovered; }
    bool IsLeftPressed() const { return m_LeftPressed; }
    bool IsRightPressed() const { return m_RightPressed; }
    bool IsMiddlePressed() const { return m_MiddlePressed; }

    virtual void OnCollisionEnter(entt::entity other) {}
    virtual void OnCollisionStay(entt::entity other) {}
    virtual void OnCollisionExit(entt::entity other) {}

    virtual void OnTriggerEnter(entt::entity other) {}
    virtual void OnTriggerStay(entt::entity other) {}
    virtual void OnTriggerExit(entt::entity other) {}

    bool GetAction(const std::string &name);
    bool GetActionDown(const std::string &name);
    bool GetActionUp(const std::string &name);

    void LoadScene(const std::string &path);

    void SetTimeScale(float scale);
    float GetTimeScale() const;
    float GetRealDeltaTime() const;

    class SoundManager &GetSoundManager();
    class ResourceManager &GetResourceManager();
    class AppHandler &GetAppHandler();
    class SceneManager &GetSceneManager();
    class InputManager &GetInputManager();
    class KeyboardManager &GetKeyboard();
    class MouseManager &GetMouse();

    template <typename T>
    T &GetComponent()
    {
        return m_Scene->registry.get<T>(m_Entity);
    }

    template <typename T>
    bool HasComponent()
    {
        return m_Scene->registry.all_of<T>(m_Entity);
    }

    template <typename T>
    T *GetScript(entt::entity targetEntity)
    {
        if (m_Scene->registry.all_of<ScriptComponent>(targetEntity))
        {
            auto &nsc = m_Scene->registry.get<ScriptComponent>(targetEntity);
            return dynamic_cast<T *>(nsc.instance);
        }
        return nullptr;
    }

    void SetHovered(bool hovered) { m_IsHovered = hovered; }

    bool &GetLeftPressedRef() { return m_LeftPressed; }
    float &GetLeftHoldTimeRef() { return m_LeftHoldTime; }

    bool &GetRightPressedRef() { return m_RightPressed; }
    float &GetRightHoldTimeRef() { return m_RightHoldTime; }

    bool &GetMiddlePressedRef() { return m_MiddlePressed; }
    float &GetMiddleHoldTimeRef() { return m_MiddleHoldTime; }

    const std::vector<KeyBinding> &GetKeyBindings() const { return m_KeyBindings; }

protected:
    entt::entity m_Entity;
    Scene *m_Scene = nullptr;
    Application *m_App = nullptr;

private:
    bool m_Enabled = true;
    bool m_RunWhenPaused = false;

    bool m_IsHovered = false;

    bool m_LeftPressed = false;
    float m_LeftHoldTime = 0.0f;

    bool m_RightPressed = false;
    float m_RightHoldTime = 0.0f;

    bool m_MiddlePressed = false;
    float m_MiddleHoldTime = 0.0f;

    std::vector<KeyBinding> m_KeyBindings;
};