#pragma once

#include <editor/i_editor_module.h>

#ifdef ENABLE_EDITOR

class GeneralEditorModule : public IEditorModule
{
public:
    GeneralEditorModule() = default;
    ~GeneralEditorModule() override = default;

    void ProcessInput(KeyboardManager& keyboard) override;

private:
    bool m_F11Pressed = false;
    bool m_F12Pressed = false;
};

#endif
