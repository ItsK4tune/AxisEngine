#pragma once

#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <functional>

class Application;

#ifdef ENABLE_EDITOR

class RenderEditorModule : public IEditorModule
{
public:
    RenderEditorModule() = default;
    ~RenderEditorModule() override = default;

    void ProcessInput(KeyboardManager& keyboard) override;

private:
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_F6Pressed = false;
    bool m_F7Pressed = false;
};

#endif
