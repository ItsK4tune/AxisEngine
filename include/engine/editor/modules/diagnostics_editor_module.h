#pragma once

#include <editor/i_editor_module.h>
#include <cstdint>

#ifdef ENABLE_EDITOR

class DiagnosticsEditorModule : public IEditorModule
{
public:
    DiagnosticsEditorModule() = default;
    ~DiagnosticsEditorModule() override = default;

    void Shutdown() override;
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

private:
    bool m_F8Pressed = false;
    bool m_F9Pressed = false;

    uint32_t m_LineVAO = 0;
    uint32_t m_LineVBO = 0;
};

#endif
