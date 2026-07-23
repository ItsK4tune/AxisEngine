#include <editor/panels/tools_panel.h>

#ifdef ENABLE_EDITOR
#include <core/app/runtime_core.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/time_service.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/mouse_manager.h>
#include <imgui.h>

void ToolsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (!cm)
    {
        ImGui::End();
        return;
    }

    auto conf = cm->GetConfig();
    uint32_t changeMask = ConfigChangedEvent::None;
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        auto& mouse = io->GetMouse();
        CursorMode mode = mouse.GetCursorMode();
        const char* modeStr = (mode == CursorMode::Locked)     ? "Locked (FPS)"
                              : (mode == CursorMode::Editor)   ? "Editor"
                              : (mode == CursorMode::Disabled) ? "Disabled"
                              : (mode == CursorMode::Normal)   ? "Normal"
                              : (mode == CursorMode::Hidden)   ? "Hidden"
                                                               : "LockedHidden";
        ImGui::Text("Cursor: %s", modeStr);

        bool editorMode = mouse.IsEditorMode();
        if (ImGui::Checkbox("Editor Cursor (F10)", &editorMode))
        {
            if (editorMode)
                mouse.EnterEditorMode();
            else
                mouse.ExitEditorMode();
        }
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // No Texture mode removed from editor UI
        if (ImGui::Checkbox("Enable Shadows", &conf.shadow.shadowsEnabled))
            changeMask |= ConfigChangedEvent::Graphics;
        if (ImGui::SliderFloat("Skybox Intensity", &conf.render.skyboxIntensity, 0.0f, 2.0f))
            changeMask |= ConfigChangedEvent::Graphics;
    }

    auto* sysMgr = ServiceLocator::Instance().Resolve<SystemManager>();
    if (sysMgr)
    {
        if (ImGui::CollapsingHeader("System Controls", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* skyboxSys = sysMgr->GetSystem("SkyboxRenderSystem");
            if (skyboxSys)
            {
                bool skyboxEnabled = skyboxSys->IsEnabled();
                if (ImGui::Checkbox("Enable Skybox (F4)", &skyboxEnabled))
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"SkyboxRenderSystem", skyboxEnabled});
                }
            }

            auto* ppSys = sysMgr->GetSystem("PostProcessSystem");
            if (ppSys)
            {
                bool ppEnabled = ppSys->IsEnabled();
                if (ImGui::Checkbox("Enable Post Process (F6)", &ppEnabled))
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"PostProcessSystem", ppEnabled});
                }
            }

            auto* audioSys = sysMgr->GetSystem("AudioSystem");
            if (audioSys)
            {
                bool audioEnabled = audioSys->IsEnabled();
                if (ImGui::Checkbox("Enable Audio", &audioEnabled))
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"AudioSystem", audioEnabled});
                }
            }

            auto* uiSys = sysMgr->GetSystem("UIRenderSystem");
            if (uiSys)
            {
                bool uiEnabled = uiSys->IsEnabled();
                if (ImGui::Checkbox("Enable UI Rendering", &uiEnabled))
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"UIRenderSystem", uiEnabled});
                }
            }
        }
    }

    auto& sl = ServiceLocator::Instance();
    auto* core = sl.Resolve<RuntimeCore>();
    auto* timer = sl.Resolve<TimeService>();
    if (core && timer)
    {
        if (ImGui::CollapsingHeader("Engine Loop", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool paused = timer->IsPaused();
            if (ImGui::Checkbox("Pause Engine (F11)", &paused))
            {
                core->GetEngineLoop().SetPaused(paused);
            }
            float timeScale = timer->GetTimeScale();
            if (ImGui::SliderFloat("Time Scale (F12)", &timeScale, 0.25f, 2.0f))
            {
                timer->SetTimeScale(timeScale);
            }
        }
    }

    if (ImGui::CollapsingHeader("Debug Vis", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Checkbox("Physics Debug (F7)", &conf.debug.physicsDebug))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Editor UI Enabled", &conf.debug.uiEnabled))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Entity Names (F1)", &conf.debug.entityNames))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Gizmos (F2)", &conf.debug.gizmos))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Light Gizmos (F3)", &conf.debug.lightGizmos))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Audio Debug", &conf.debug.audioDebug))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Particle Debug", &conf.debug.particleDebug))
            changeMask |= ConfigChangedEvent::Debug;
        if (ImGui::Checkbox("Grid Snapping (Ctrl+G)", &conf.debug.gridSnapEnabled))
            changeMask |= ConfigChangedEvent::Debug;
        if (conf.debug.gridSnapEnabled)
        {
            ImGui::Indent();
            if (ImGui::SliderFloat("Translation Snap", &conf.debug.gridSnapTranslation, 0.1f, 10.0f))
                changeMask |= ConfigChangedEvent::Debug;
            if (ImGui::SliderFloat("Rotation Snap", &conf.debug.gridSnapRotation, 1.0f, 90.0f))
                changeMask |= ConfigChangedEvent::Debug;
            if (ImGui::SliderFloat("Scale Snap", &conf.debug.gridSnapScale, 0.05f, 5.0f))
                changeMask |= ConfigChangedEvent::Debug;
            ImGui::Unindent();
        }
        if (ImGui::Checkbox("Grid Indicator (Ctrl+H)", &conf.debug.gridIndicatorEnabled))
            changeMask |= ConfigChangedEvent::Debug;
    }

    if (changeMask != ConfigChangedEvent::None)
    {
        cm->UpdateConfig(conf, changeMask);
    }

    ImGui::End();
}
#endif
