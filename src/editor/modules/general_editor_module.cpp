#include <editor/modules/general_editor_module.h>

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
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>

void GeneralEditorModule::Initialize()
{
}

void GeneralEditorModule::OnUpdate(float dt)
{
    if (!m_Enabled)
        return;

    m_FpsTimer += dt;
    m_FrameCount++;
    if (m_FpsTimer >= 1.0f)
    {
        m_CurrentFps = (float)m_FrameCount / m_FpsTimer;
        m_CurrentFrameTime = (m_FpsTimer / m_FrameCount) * 1000.0f;
        m_FpsTimer = 0.0f;
        m_FrameCount = 0;
    }
}

void GeneralEditorModule::Render(Scene& scene)
{
}

void GeneralEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F11, m_F11Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (!shift)
        {
            auto& sl = ServiceLocator::Instance();
            auto* core = sl.Resolve<RuntimeCore>();
            auto* timer = sl.Resolve<TimeService>();

            if (core && timer)
            {
                bool nextPause = !timer->IsPaused();
                core->GetEngineLoop().SetPaused(nextPause);
            }
        }
    });

    ProcessKey(keyboard, Key::F12, m_F12Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);

        if (shift)
        {
            auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
            if (io)
            {
                auto& mouse = io->GetMouse();
                CursorMode current = mouse.GetCursorMode();
                CursorMode next = CursorMode::Normal;
                std::string modeName = "Normal";

                switch (current)
                {
                    case CursorMode::Normal:
                        next = CursorMode::Hidden;
                        modeName = "Hidden";
                        break;
                    case CursorMode::Hidden:
                        next = CursorMode::Locked;
                        modeName = "Locked";
                        break;
                    case CursorMode::Locked:
                        next = CursorMode::LockedHidden;
                        modeName = "LockedHidden";
                        break;
                    case CursorMode::LockedHidden:
                        next = CursorMode::Normal;
                        modeName = "Normal";
                        break;
                    default:
                        next = CursorMode::Normal;
                        modeName = "Normal";
                        break;
                }

                mouse.SetCursorMode(next);
            }
        }
        else
        {
            auto& sl = ServiceLocator::Instance();
            auto* timer = sl.Resolve<TimeService>();
            auto* core = sl.Resolve<RuntimeCore>();

            if (timer && core)
            {
                float current = timer->GetTimeScale();
                float next = 1.0f;
                if (abs(current - 0.25f) < 0.01f)
                    next = 0.5f;
                else if (abs(current - 0.5f) < 0.01f)
                    next = 1.0f;
                else if (abs(current - 1.0f) < 0.01f)
                    next = 1.5f;
                else if (abs(current - 1.5f) < 0.01f)
                    next = 2.0f;
                else if (abs(current - 2.0f) < 0.01f)
                    next = 0.25f;
                else
                    next = 1.0f;

                core->GetEngineLoop().SetTimeScale(next);
            }
        }
    });
}

void GeneralEditorModule::ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState,
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
