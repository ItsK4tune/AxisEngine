#pragma once

#include <core/type/app_config.h>
#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <functional>
#include <string>

struct Scene;

class Application;

#ifdef ENABLE_EDITOR

class GeneralEditorModule : public IEditorModule
{
public:
    GeneralEditorModule() = default;
    ~GeneralEditorModule() override = default;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }
    std::string GetModuleName() const override
    {
        return "GeneralEditorModule";
    }

private:
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_Enabled = true;

    bool m_F11Pressed = false;
    bool m_F12Pressed = false;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;
};

#endif
