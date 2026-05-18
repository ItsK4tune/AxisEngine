#include <editor/modules/render_editor_module.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <platform/logic/input_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <iostream>

RenderEditorModule::RenderEditorModule()
{
}
RenderEditorModule::~RenderEditorModule()
{
}

void RenderEditorModule::Initialize()
{
}

void RenderEditorModule::OnUpdate(float dt)
{
}

void RenderEditorModule::Render(Scene& scene)
{
}

void RenderEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F6, m_F6Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift)
        {
            auto& sl = ServiceLocator::Instance();
            static bool skyboxEnabled = true;
            skyboxEnabled = !skyboxEnabled;
            EventManager::Instance().Publish(SystemEnabledEvent{"SkyboxRenderSystem", skyboxEnabled});
        }
    });

    ProcessKey(keyboard, Key::F7, m_F7Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift)
        {
            auto& sl = ServiceLocator::Instance();
            auto* shadowSys = sl.Resolve<IShadowService>();
            bool shadow = shadowSys ? !shadowSys->IsShadowsEnabled() : false;
            if (shadowSys)
                shadowSys->SetEnableShadows(shadow);
        }
    });
}

void RenderEditorModule::ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState,
                                    std::function<void()> action)
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
