#include <editor/modules/physics_editor_module.h>

#ifdef ENABLE_EDITOR

#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <editor/editor_shortcut.h>
#include <platform/interface/i_ui_input_capture.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>

void PhysicsEditorModule::Render(Scene& scene)
{
    if (!m_Enabled)
        return;
    const auto* config = ServiceLocator::Instance().Resolve<ConfigManager>();
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!config || !io || !config->GetConfigSnapshot()->debug.physicsDebug)
        return;
    EventManager::Instance().Publish(
        PhysicsDebugRenderEvent{&scene, io->GetMonitorManager().GetWidth(), io->GetMonitorManager().GetHeight()});
}

void PhysicsEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;
    auto& services = ServiceLocator::Instance();
    const auto* capture = services.Resolve<IUIInputCapture>();
    const bool blocked = capture && capture->WantsTextInput();
    if (!IsEditorShortcutPressed(keyboard, Key::F7, EditorModifier::None, m_F7Pressed, blocked))
        return;
    if (auto* config = services.Resolve<ConfigManager>())
    {
        auto value = config->GetConfig();
        value.debug.physicsDebug = !value.debug.physicsDebug;
        config->UpdateConfig(value, ConfigChangedEvent::Debug);
    }
}

#endif
