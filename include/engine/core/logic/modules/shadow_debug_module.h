#pragma once

#include <core/interface/i_debug_module.h>
#include <string>

class Application;

#ifdef ENABLE_DEBUG_SYSTEM



class ShadowDebugModule : public IDebugModule
{
public:
    ShadowDebugModule();
    ~ShadowDebugModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "ShadowDebugModule"; }

private:
    bool m_Enabled = true;
};

#endif