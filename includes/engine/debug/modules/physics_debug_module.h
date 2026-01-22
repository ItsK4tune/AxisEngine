#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <debug/modules/i_debug_module.h>
#include <functional>
#include <string>

class Application;

class PhysicsDebugModule : public IDebugModule
{
public:
    PhysicsDebugModule();
    ~PhysicsDebugModule() override;

    void Init(Application *app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "PhysicsDebugModule"; }

    bool IsPhysicsDebugEnabled() const { return m_ShowPhysicsDebug; }

private:
    void TogglePhysicsDebug();
    void ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action);

    Application *m_App = nullptr;
    bool m_Enabled = true;

    bool m_F8Pressed = false;
    bool m_F9Pressed = false;

    bool m_ShowPhysicsDebug = false;
    bool m_ShowAudioDebug = false;
    bool m_ShowParticleDebug = false;
};

#endif
