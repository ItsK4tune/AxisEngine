#include <editor/modules/render_editor_module.h>
#include <editor/editor_shortcut.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <platform/logic/input_manager.h>
#include <platform/interface/i_ui_input_capture.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <iostream>

void RenderEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    const auto* capture = sl.Resolve<IUIInputCapture>();
    const bool inputBlocked = capture && capture->WantsTextInput();
    if (IsEditorShortcutPressed(keyboard, Key::F4, EditorModifier::None, m_F4Pressed, inputBlocked))
    {
        auto* sysMgr = sl.Resolve<SystemManager>();
        auto* skyboxSys = sysMgr ? sysMgr->GetSystem("SkyboxRenderSystem") : nullptr;
        if (skyboxSys)
            EventManager::Instance().Publish(SystemEnabledEvent{"SkyboxRenderSystem", !skyboxSys->IsEnabled()});
    }

    if (IsEditorShortcutPressed(keyboard, Key::F5, EditorModifier::None, m_F5Pressed, inputBlocked))
    {
        auto* shadowSys = sl.Resolve<IShadowService>();
        bool shadow = shadowSys ? !shadowSys->IsShadowsEnabled() : false;
        if (shadowSys)
            shadowSys->SetEnableShadows(shadow);
    }
}

#endif
