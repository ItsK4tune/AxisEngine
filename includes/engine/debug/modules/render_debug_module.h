#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <debug/modules/i_debug_module.h>
#include <functional>
#include <string>

class Application;

class RenderDebugModule : public IDebugModule
{
public:
    RenderDebugModule();
    ~RenderDebugModule() override;

    void Init(Application *app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "RenderDebugModule"; }

    bool IsWireframeMode() const { return m_WireframeMode; }
    bool IsNoTextureMode() const { return m_NoTextureMode; }

private:
    void ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action);

    Application *m_App = nullptr;
    bool m_Enabled = true;

    bool m_F6Pressed = false;
    bool m_F7Pressed = false;
    bool m_F9Pressed = false;

    bool m_WireframeMode = false;
    bool m_NoTextureMode = false;
};

#endif
