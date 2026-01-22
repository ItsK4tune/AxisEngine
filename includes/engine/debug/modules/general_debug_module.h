#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <interface/debug_module.h>
#include <functional>
#include <string>

class Application;

class GeneralDebugModule : public IDebugModule
{
public:
    GeneralDebugModule();
    ~GeneralDebugModule() override;

    void Init(Application *app) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "GeneralDebugModule"; }

private:
    void LogDevices();
    void LogSceneGraph();
    void LogControls();
    void LogStats();
    void LogEntityStats();
    void ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action);

    Application *m_App = nullptr;
    bool m_Enabled = true;

    bool m_F1Pressed = false;
    bool m_F2Pressed = false;
    bool m_F3Pressed = false;
    bool m_F4Pressed = false;
    bool m_F5Pressed = false;
    bool m_F11Pressed = false;
    bool m_F12Pressed = false;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::string m_GpuName;
    std::string m_CpuName;
};

#endif
