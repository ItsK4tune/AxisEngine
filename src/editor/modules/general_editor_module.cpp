#include <editor/modules/general_editor_module.h>
#include <editor/editor_shortcut.h>

#ifdef ENABLE_EDITOR

#include <audio/logic/audio_service.h>
#include <core/app/runtime_core.h>
#include <core/logic/service_locator.h>
#include <core/logic/time_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <platform/interface/input_codes.h>
#include <platform/interface/i_ui_input_capture.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>
#include <cmath>

void GeneralEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    const auto* capture = sl.Resolve<IUIInputCapture>();
    const bool inputBlocked = capture && capture->WantsTextInput();

    if (IsEditorShortcutPressed(keyboard, Key::F11, EditorModifier::None, m_F11Pressed, inputBlocked))
    {
        auto* core = sl.Resolve<RuntimeCore>();
        auto* timer = sl.Resolve<TimeService>();
        if (core && timer)
        {
            bool nextPause = !timer->IsPaused();
            core->GetEngineLoop().SetPaused(nextPause);
        }
    }

    if (IsEditorShortcutPressed(keyboard, Key::F12, EditorModifier::None, m_F12Pressed, inputBlocked))
    {
        auto* timer = sl.Resolve<TimeService>();
        auto* core = sl.Resolve<RuntimeCore>();
        if (timer && core)
        {
            float current = timer->GetTimeScale();
            float next = 1.0f;
            if (std::abs(current - 0.25f) < 0.01f)
                next = 0.5f;
            else if (std::abs(current - 0.5f) < 0.01f)
                next = 1.0f;
            else if (std::abs(current - 1.0f) < 0.01f)
                next = 1.5f;
            else if (std::abs(current - 1.5f) < 0.01f)
                next = 2.0f;
            else if (std::abs(current - 2.0f) < 0.01f)
                next = 0.25f;
            core->GetEngineLoop().SetTimeScale(next);
        }
    }
}

#endif
