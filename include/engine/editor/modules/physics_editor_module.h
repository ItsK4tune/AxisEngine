#pragma once

#include <editor/i_editor_module.h>

#ifdef ENABLE_EDITOR

class PhysicsEditorModule final : public IEditorModule
{
public:
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

private:
    bool m_F7Pressed = false;
};

#endif
