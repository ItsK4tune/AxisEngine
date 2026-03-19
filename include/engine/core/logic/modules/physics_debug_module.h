#pragma once

#include <core/interface/i_debug_module.h>
#include <functional>
#include <platform/interface/input_codes.h>
#include <string>

class Application;

#ifdef ENABLE_DEBUG_SYSTEM



class PhysicsDebugModule : public IDebugModule
{
public:
    PhysicsDebugModule();
    ~PhysicsDebugModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "PhysicsDebugModule"; }
    int GetRenderOrder() const override { return 10; }

    bool IsPhysicsDebugEnabled() const { return m_ShowPhysicsDebug; }

private:
    void TogglePhysicsDebug();
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    bool m_Enabled = true;

    bool m_F8Pressed = false;
    bool m_F9Pressed = false;

    bool m_ShowPhysicsDebug = false;
    bool m_ShowAudioDebug = false;
    bool m_ShowParticleDebug = false;
};

#endif