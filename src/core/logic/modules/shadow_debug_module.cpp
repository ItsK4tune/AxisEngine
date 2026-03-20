#include <core/logic/modules/shadow_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/app/application.h>
#include <render/interface/i_graphics_context.h>
#include <ecs/logic/render_system.h>
#include <core/logic/debug_system.h>
#include <iostream>
#include <platform/logic/input_manager.h>
#include <core/logic/service_locator.h>

ShadowDebugModule::ShadowDebugModule() {}
ShadowDebugModule::~ShadowDebugModule() {}

void ShadowDebugModule::Initialize()
{
}

void ShadowDebugModule::OnUpdate(float dt)
{
}

void ShadowDebugModule::Render(Scene &scene)
{
    if (!m_Enabled)
        return;
}

void ShadowDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;
}

void ShadowDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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
