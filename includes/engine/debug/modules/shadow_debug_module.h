#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <interface/debug/i_debug_module.h>
#include <string>

class Application;

class ShadowDebugModule : public IDebugModule
{
public:
    ShadowDebugModule();
    ~ShadowDebugModule() override;

    void Init(std::shared_ptr<Application> app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "ShadowDebugModule"; }

private:
    std::shared_ptr<Application> m_App = nullptr;
    bool m_Enabled = true;
};

#endif
