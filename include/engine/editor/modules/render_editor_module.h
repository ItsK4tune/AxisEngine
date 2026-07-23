#pragma once

#include <editor/i_editor_module.h>

#ifdef ENABLE_EDITOR

class RenderEditorModule : public IEditorModule
{
public:
    RenderEditorModule() = default;
    ~RenderEditorModule() override = default;

    void ProcessInput(KeyboardManager& keyboard) override;

private:
    bool m_F4Pressed = false;
    bool m_F5Pressed = false;
};

#endif
