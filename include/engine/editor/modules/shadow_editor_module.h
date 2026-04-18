#pragma once

#include <editor/i_editor_module.h>
#include <string>
#include <functional>
#include <platform/interface/input_codes.h>

class Application;
class KeyboardManager;

#ifdef ENABLE_EDITOR



class ShadowEditorModule : public IEditorModule
{
public:
    ShadowEditorModule();
    ~ShadowEditorModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;
    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "ShadowEditorModule"; }

private:
    bool m_Enabled = true;
};

#endif