#pragma once

#include <core/type/app_config.h>
#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <functional>

struct Scene;

class Application;

#ifdef ENABLE_EDITOR

class GeneralEditorModule : public IEditorModule
{
public:
    GeneralEditorModule() = default;
    ~GeneralEditorModule() override = default;

    void ProcessInput(KeyboardManager& keyboard) override;

private:
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_F11Pressed = false;
    bool m_F12Pressed = false;
};

#endif
