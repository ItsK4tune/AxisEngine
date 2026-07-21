#pragma once

#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <functional>
#include <cstdint>

class Application;

#ifdef ENABLE_EDITOR

class PhysicsEditorModule : public IEditorModule
{
public:
    PhysicsEditorModule() = default;
    ~PhysicsEditorModule() override = default;

    void Shutdown() override;
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

private:
    void TogglePhysicsDebug();
    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_F8Pressed = false;
    bool m_F9Pressed = false;

    uint32_t m_LineVAO = 0;
    uint32_t m_LineVBO = 0;
};

#endif
