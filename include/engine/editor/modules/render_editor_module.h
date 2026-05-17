#pragma once

#include <editor/i_editor_module.h>
#include <functional>
#include <platform/interface/input_codes.h>
#include <string>

class Application;

#ifdef ENABLE_EDITOR



class RenderEditorModule : public IEditorModule
{
public:
    RenderEditorModule();
    ~RenderEditorModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "RenderEditorModule"; }

    bool IsWireframeMode() const { return m_WireframeMode; }

private:
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    bool m_Enabled = true;

    bool m_F6Pressed = false;
    bool m_F7Pressed = false;

    bool m_WireframeMode = false;
};

#endif