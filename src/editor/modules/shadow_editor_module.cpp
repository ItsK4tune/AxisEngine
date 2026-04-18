#include <editor/modules/shadow_editor_module.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <render/interface/i_graphics_context.h>
#include <ecs/logic/render_system.h>
#include <editor/editor_system.h>
#include <iostream>
#include <platform/logic/input_manager.h>
#include <core/logic/service_locator.h>

ShadowEditorModule::ShadowEditorModule() {}
ShadowEditorModule::~ShadowEditorModule() {}

void ShadowEditorModule::Initialize()
{
}

void ShadowEditorModule::OnUpdate(float dt)
{
}

void ShadowEditorModule::Render(Scene &scene)
{
    if (!m_Enabled)
        return;
}

void ShadowEditorModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;
}

void ShadowEditorModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
{
    if (keyboard.GetKey(key))
    {
        if (!pressedState)
        {
            action();
            pressedState = true;
        }
    }
    else
    {
        pressedState = false;
    }
}

#endif
