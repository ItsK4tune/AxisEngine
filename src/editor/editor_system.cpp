#include <editor/editor_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <editor/i_editor_module.h>
#include <editor/modules/camera_editor_module.h>
#include <editor/modules/general_editor_module.h>
#include <editor/modules/gizmo_editor_module.h>
#include <editor/modules/physics_editor_module.h>
#include <editor/modules/render_editor_module.h>

#ifdef ENABLE_EDITOR

#include <audio/logic/audio_service.h>
#include <core/app/runtime_core.h>
#include <core/logic/log_manager.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <physics/interface/i_physics_world.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/type/scene_events.h>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

// Panel includes
#include <core/logic/event_manager.h>
#include <core/logic/time_service.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <editor/panels/console_panel.h>
#include <editor/panels/file_hierarchy_panel.h>
#include <editor/panels/help_panel.h>
#include <editor/panels/profiler_panel.h>
#include <editor/panels/resource_browser_panel.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <editor/panels/settings_panel.h>
#include <editor/panels/stats_panel.h>
#include <editor/panels/state_panel.h>
#include <editor/panels/network_panel.h>
#include <editor/panels/tools_panel.h>

EditorSystem::EditorSystem() : m_PreHoverCursorMode(CursorMode::Normal)
{
}
EditorSystem::~EditorSystem()
{
}

REGISTER_SYSTEM(EditorSystem)

void EditorSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto* res = sl.Resolve<ResourceManager>();
    if (!res)
    {
        LOGGER_WARN("EditorSystem") << "Skipping full initialization (missing ResourceManager)";
        return;
    }

    if (!res->GetUIModel("debug_sys_model"))
    {
        res->CreateUIModel("debug_sys_model", ::UIType::Text);
    }
    auto debugFont = res->GetFont("debug_font");
    auto textShader = res->GetShader("debug_text");
    auto textQuad = res->GetUIModel("debug_sys_model");

    LOGGER_INFO("EditorSystem") << "Initializing legacy debug modules for backend...";

    auto generalModule = std::make_unique<GeneralEditorModule>();
    generalModule->Initialize();
    m_Modules.push_back(std::move(generalModule));

    auto renderModule = std::make_unique<RenderEditorModule>();
    renderModule->Initialize();
    m_Modules.push_back(std::move(renderModule));

    auto physicsModule = std::make_unique<PhysicsEditorModule>();
    physicsModule->Initialize();
    m_Modules.push_back(std::move(physicsModule));

    auto gizmoModule = std::make_unique<GizmoEditorModule>();
    gizmoModule->Initialize();
    gizmoModule->SetSharedResources(debugFont, textShader, textQuad);
    m_Modules.push_back(std::move(gizmoModule));

    auto cameraModule = std::make_unique<CameraEditorModule>();
    cameraModule->Initialize();
    m_Modules.push_back(std::move(cameraModule));

    // Wait, let's setup ImGui
    auto* ioHandler = sl.Resolve<IOHandler>();
    if (ioHandler)
    {
        auto* window = ioHandler->GetMonitorManager().GetWindow();
        if (window && window->GetNativeWindow())
        {
            m_ImGuiLayer.Initialize(reinterpret_cast<GLFWwindow*>(window->GetNativeWindow()));
        }
    }

    // Register Panels
    m_Panels.push_back(std::make_unique<SceneHierarchyPanel>());
    m_Panels.push_back(std::make_unique<ResourceBrowserPanel>());
    m_Panels.push_back(std::make_unique<ToolsPanel>());
    m_Panels.push_back(std::make_unique<SettingsPanel>());
    m_Panels.push_back(std::make_unique<FileHierarchyPanel>());
    m_Panels.push_back(std::make_unique<HelpPanel>());
    m_Panels.push_back(std::make_unique<ConsolePanel>());
    m_Panels.push_back(std::make_unique<ProfilerPanel>());
    m_Panels.push_back(std::make_unique<StatePanel>());
    m_Panels.push_back(std::make_unique<NetworkPanel>());

    for (auto& panel : m_Panels)
    {
        panel->Initialize();
    }

    LOGGER_INFO("EditorSystem") << "Initialized completely.";

    // Wire log callback to console panel
    LogManager::Instance().SetEditorLogCallback([](LogType type, const std::string& tag, const std::string& msg) {
        if (ConsolePanel::s_Instance)
        {
            ConsolePanel::s_Instance->PushLog(type, tag, msg);
        }
    });

    auto& globalScene = ServiceLocator::Instance().Require<Scene>();
    PushUndoState(globalScene);
}

void EditorSystem::Shutdown()
{
    m_ImGuiLayer.Shutdown();
}

void EditorSystem::OnUpdate(float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto* ioHandler = sl.Resolve<IOHandler>();
    if (!ioHandler)
        return;

    KeyboardManager& kb = ioHandler->GetKeyboard();
    // Proxy legacy keys
    for (auto& module : m_Modules)
    {
        module->ProcessInput(kb);
        if (module->IsEnabled())
        {
            module->OnUpdate(dt);
        }
    }

    for (auto& panel : m_Panels)
    {
        if (panel->IsOpen())
        {
            panel->OnUpdate(dt);
        }
    }

    // Cursor & input management
    auto mode = ioHandler->GetMouse().GetCursorMode();
    if (mode == CursorMode::Hidden || mode == CursorMode::Locked || mode == CursorMode::LockedHidden)
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
    else
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }

    // Note: Camera controller already checks ImGui::WantCaptureMouse to skip input.
    // No cursor mode manipulation needed here — it breaks mouse in overlay/sample mode.

    // === Keyboard Shortcuts ===
    bool ctrl = kb.GetKey(Key::LeftControl) || kb.GetKey(Key::RightControl);
    bool shift = kb.GetKey(Key::LeftShift) || kb.GetKey(Key::RightShift);

    // Ctrl+S — Save all scenes
    if (ctrl && kb.GetKey(Key::S) && !m_CtrlSPressed)
    {
        m_CtrlSPressed = true;
        auto& sm = sl.Require<SceneManager>();
        auto& globalScene = sl.Require<Scene>();
        auto* resourceManager = sl.Resolve<ResourceManager>();
        if (resourceManager)
        {
            for (const auto& rec : sm.GetAllScenes())
            {
                if (!rec.filePath.empty())
                {
                    SceneSerializer::Serialize(rec.filePath, globalScene, *resourceManager, rec.name);
                    LOGGER_INFO("EditorSystem") << "Saved scene: " << rec.name;
                }
            }
        }
    }
    if (!kb.GetKey(Key::S))
        m_CtrlSPressed = false;

    // Ctrl+Z — Undo
    static bool z_pressed = false;
    if (ctrl && kb.GetKey(Key::Z) && !z_pressed)
    {
        z_pressed = true;
        auto& globalScene = sl.Require<Scene>();
        if (shift)
            PerformRedo(globalScene);
        else
            PerformUndo(globalScene);
    }
    if (!kb.GetKey(Key::Z))
        z_pressed = false;

    static bool y_pressed = false;
    if (ctrl && kb.GetKey(Key::Y) && !y_pressed)
    {
        y_pressed = true;
        auto& globalScene = sl.Require<Scene>();
        PerformRedo(globalScene);
    }
    if (!kb.GetKey(Key::Y))
        y_pressed = false;

    // Ctrl+D — Duplicate selected entity
    // (handled in SceneHierarchyPanel via shared selection)

    // --- Nudge Tool ---
    static float s_NudgeTimer = 0.0f;
    s_NudgeTimer -= dt;
    bool alt = kb.GetKey(Key::LeftAlt) || kb.GetKey(Key::RightAlt);
    if (alt && SceneHierarchyPanel::s_SelectedEntity != entt::null && s_NudgeTimer <= 0.0f)
    {
        auto& globalScene = sl.Require<Scene>();
        auto& reg = globalScene.registry;
        if (reg.valid(SceneHierarchyPanel::s_SelectedEntity))
        {
            auto cm = sl.Resolve<ConfigManager>();
            if (cm)
            {
                const auto& conf = cm->GetConfig();
                float tSnap = conf.debug.gridSnapTranslation;
                float rSnap = glm::radians(conf.debug.gridSnapRotation);
                float sSnap = conf.debug.gridSnapScale;
                bool snapEnabled = conf.debug.gridSnapEnabled;
                if (!snapEnabled)
                {
                    tSnap = 0.05f;
                    rSnap = glm::radians(1.0f);
                    sSnap = 0.05f;
                }

                bool nudged = false;
                if (ctrl && !shift && reg.all_of<RotationComponent>(SceneHierarchyPanel::s_SelectedEntity))
                {  // Rotate (Ctrl + Alt + Keys)
                    auto& rot = reg.get<RotationComponent>(SceneHierarchyPanel::s_SelectedEntity);
                    if (kb.GetKey(Key::Left))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(0, 1, 0));
                        nudged = true;
                    }
                    if (kb.GetKey(Key::Right))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(0, 1, 0));
                        nudged = true;
                    }
                    if (kb.GetKey(Key::Up))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(1, 0, 0));
                        nudged = true;
                    }
                    if (kb.GetKey(Key::Down))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(1, 0, 0));
                        nudged = true;
                    }
                    if (kb.GetKey(Key::PageUp))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(0, 0, 1));
                        nudged = true;
                    }
                    if (kb.GetKey(Key::PageDown))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(0, 0, 1));
                        nudged = true;
                    }
                    if (nudged)
                        rot.prev = rot.value;
                }
                else if (shift && reg.all_of<ScaleComponent>(SceneHierarchyPanel::s_SelectedEntity))
                {  // Scale (Shift + Alt + Keys)
                    auto& scale = reg.get<ScaleComponent>(SceneHierarchyPanel::s_SelectedEntity);
                    if (snapEnabled)
                    {
                        if (kb.GetKey(Key::Left))
                        {
                            scale.value.x = std::round((scale.value.x - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Right))
                        {
                            scale.value.x = std::round((scale.value.x + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Up))
                        {
                            scale.value.z = std::round((scale.value.z - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Down))
                        {
                            scale.value.z = std::round((scale.value.z + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageUp))
                        {
                            scale.value.y = std::round((scale.value.y + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageDown))
                        {
                            scale.value.y = std::round((scale.value.y - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                    }
                    else
                    {
                        if (kb.GetKey(Key::Left))
                        {
                            scale.value.x -= sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Right))
                        {
                            scale.value.x += sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Up))
                        {
                            scale.value.z -= sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Down))
                        {
                            scale.value.z += sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageUp))
                        {
                            scale.value.y += sSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageDown))
                        {
                            scale.value.y -= sSnap;
                            nudged = true;
                        }
                    }
                    if (nudged)
                    {
                        if (scale.value.x < 0.001f)
                            scale.value.x = 0.001f;
                        if (scale.value.y < 0.001f)
                            scale.value.y = 0.001f;
                        if (scale.value.z < 0.001f)
                            scale.value.z = 0.001f;
                        scale.prev = scale.value;
                    }
                }
                else if (!ctrl && !shift && reg.all_of<PositionComponent>(SceneHierarchyPanel::s_SelectedEntity))
                {  // Translate (Alt + Keys)
                    auto& pos = reg.get<PositionComponent>(SceneHierarchyPanel::s_SelectedEntity);
                    if (snapEnabled)
                    {
                        if (kb.GetKey(Key::Left))
                        {
                            pos.value.x = std::round((pos.value.x - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Right))
                        {
                            pos.value.x = std::round((pos.value.x + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Up))
                        {
                            pos.value.z = std::round((pos.value.z - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Down))
                        {
                            pos.value.z = std::round((pos.value.z + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageUp))
                        {
                            pos.value.y = std::round((pos.value.y + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageDown))
                        {
                            pos.value.y = std::round((pos.value.y - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                    }
                    else
                    {
                        if (kb.GetKey(Key::Left))
                        {
                            pos.value.x -= tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Right))
                        {
                            pos.value.x += tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Up))
                        {
                            pos.value.z -= tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::Down))
                        {
                            pos.value.z += tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageUp))
                        {
                            pos.value.y += tSnap;
                            nudged = true;
                        }
                        if (kb.GetKey(Key::PageDown))
                        {
                            pos.value.y -= tSnap;
                            nudged = true;
                        }
                    }
                    if (nudged)
                        pos.prev = pos.value;
                }

                if (nudged)
                {
                    s_NudgeTimer = 0.15f;  // debounce
                    if (auto* wt = reg.try_get<WorldTransformComponent>(SceneHierarchyPanel::s_SelectedEntity))
                    {
                        wt->isDirty = true;
                    }
                }
            }
        }
    }

    // --- Debug & System Toggles ---
    static bool f1_pressed = false, f2_pressed = false, f3_pressed = false;
    static bool f4_pressed = false, f5_pressed = false, f11_pressed = false, f12_pressed = false;
    static bool g_pressed = false;
    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (cm)
    {
        auto conf = cm->GetConfig();
        bool changed = false;

        if (kb.GetKey(Key::F1) && !f1_pressed)
        {
            f1_pressed = true;
            conf.debug.entityNames = !conf.debug.entityNames;
            changed = true;
        }
        else if (!kb.GetKey(Key::F1))
            f1_pressed = false;

        if (kb.GetKey(Key::F2) && !f2_pressed)
        {
            f2_pressed = true;
            conf.debug.gizmos = !conf.debug.gizmos;
            changed = true;
        }
        else if (!kb.GetKey(Key::F2))
            f2_pressed = false;

        if (kb.GetKey(Key::F3) && !f3_pressed)
        {
            f3_pressed = true;
            conf.debug.lightGizmos = !conf.debug.lightGizmos;
            changed = true;
        }
        else if (!kb.GetKey(Key::F3))
            f3_pressed = false;

        if (kb.GetKey(Key::G))
        {
            if (!g_pressed)
            {
                g_pressed = true;
                if (ctrl)
                {
                    conf.debug.gridSnapEnabled = !conf.debug.gridSnapEnabled;
                    changed = true;
                }
                else if (shift)
                {
                    conf.debug.gridIndicatorEnabled = !conf.debug.gridIndicatorEnabled;
                    changed = true;
                }
            }
        }
        else if (!kb.GetKey(Key::G))
            g_pressed = false;

        static bool h_pressed = false;
        if (ctrl && kb.GetKey(Key::H) && !h_pressed)
        {
            h_pressed = true;
            conf.debug.gridIndicatorEnabled = !conf.debug.gridIndicatorEnabled;
            changed = true;
        }
        else if (!kb.GetKey(Key::H))
            h_pressed = false;

        if (changed)
            cm->UpdateConfig(conf, ConfigChangedEvent::All);

        // Reload Active Scene (Ctrl+R)
        static bool r_pressed = false;
        if (ctrl && kb.GetKey(Key::R) && !r_pressed)
        {
            r_pressed = true;
            auto& sm = ServiceLocator::Instance().Require<SceneManager>();
            std::string activeName = sm.GetActiveScene();
            auto* rec = sm.GetSceneByName(activeName);
            if (rec)
            {
                std::string path = rec->filePath;
                sm.UnloadScene(path);
                sm.LoadScene(path);
                sm.SetActiveScene(activeName);
                LOGGER_INFO("Editor") << "Reloaded active scene: " << activeName;
            }
        }
        else if (!kb.GetKey(Key::R))
        {
            r_pressed = false;
        }

        // Toggle Audio System (F4)
        if (kb.GetKey(Key::F4) && !f4_pressed)
        {
            f4_pressed = true;
            auto* sysMgr = ServiceLocator::Instance().Resolve<SystemManager>();
            if (sysMgr)
            {
                auto* audioSys = sysMgr->GetSystem("AudioSystem");
                if (audioSys)
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"AudioSystem", !audioSys->IsEnabled()});
                }
            }
        }
        else if (!kb.GetKey(Key::F4))
            f4_pressed = false;

        // Toggle Post Process (F5)
        if (kb.GetKey(Key::F5) && !f5_pressed)
        {
            f5_pressed = true;
            auto* sysMgr = ServiceLocator::Instance().Resolve<SystemManager>();
            if (sysMgr)
            {
                auto* ppSys = sysMgr->GetSystem("PostProcessSystem");
                if (ppSys)
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"PostProcessSystem", !ppSys->IsEnabled()});
                }
            }
        }
        else if (!kb.GetKey(Key::F5))
            f5_pressed = false;

        // Toggle Force Free Cursor (F6)
        static bool f6_pressed = false;
        if (kb.GetKey(Key::F6) && !shift && !f6_pressed)
        {
            f6_pressed = true;
            auto& mouse = ioHandler->GetMouse();
            bool nextForceFree = !mouse.IsForceFree();
            mouse.SetForceFree(nextForceFree);
            if (nextForceFree)
            {
                mouse.SetCursorMode(CursorMode::Disabled);
            }
            else
            {
                auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>();
                if (core)
                {
                    auto& sm = core->GetStateMachine();
                    State* curr = sm.GetCurrentState();
                    if (curr)
                    {
                        std::string rawName = typeid(*curr).name();
                        if (rawName.find("AimGameState") != std::string::npos)
                        {
                            mouse.SetCursorMode(CursorMode::LockedHidden);
                        }
                        else
                        {
                            mouse.SetCursorMode(CursorMode::Normal);
                        }
                    }
                }
            }
        }
        else if (!kb.GetKey(Key::F6))
            f6_pressed = false;
    }

    // Panel toggle shortcuts (Ctrl+1 to Ctrl+9, and Ctrl+0 for NetworkPanel)
    static bool s_NumKeysPressed[10] = {false};
    Key numKeys[] = {Key::_1, Key::_2, Key::_3, Key::_4, Key::_5, Key::_6, Key::_7, Key::_8, Key::_9, Key::_0};

    if (ctrl)
    {
        for (int i = 0; i < std::min((int)m_Panels.size(), 10); ++i)
        {
            if (kb.GetKey(numKeys[i]) && !s_NumKeysPressed[i])
            {
                s_NumKeysPressed[i] = true;
                m_Panels[i]->SetOpen(!m_Panels[i]->IsOpen());
            }
            else if (!kb.GetKey(numKeys[i]))
            {
                s_NumKeysPressed[i] = false;
            }
        }
    }
    else
    {
        for (int i = 0; i < 10; ++i) s_NumKeysPressed[i] = false;
    }
}

void EditorSystem::Render(Scene& scene)
{
    // Legacy modules that render into main 3D scene
    for (auto& module : m_Modules)
    {
        if (module->IsEnabled())
        {
            module->Render(scene);
        }
    }
}

void EditorSystem::RenderUIPass(Scene& scene, float width, float height, IRenderStateManager& renderState)
{
    if (!m_ImGuiLayer.IsInitialized())
        return;

    // We can also allow legacy UI render pass (e.g. Gizmos/Overlay) if needed
    // But now we prefer ImGui
    m_ImGuiLayer.BeginFrame();

    // Enable dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("AxisEngine_DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("AxisDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    DrawMenuBar();

    for (auto& panel : m_Panels)
    {
        if (panel->IsOpen())
        {
            panel->OnImGui(scene);
        }
    }

    if (m_ShowAboutPopup)
    {
        ImGui::OpenPopup("About AxisEngine");
        m_ShowAboutPopup = false;
    }

    if (ImGui::BeginPopupModal("About AxisEngine", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("AxisEngine - Advanced 5-Layer ECS Engine");
        ImGui::Text("Version: 1.0.0 (Stable)");
        ImGui::Separator();
        ImGui::Text("Developed by: Duong");
        ImGui::Text("Architecture: Core, ECS, Platform, Render, Script/Editor");
        ImGui::Text("Tech Stack: EnTT, OpenGL 3.3+, GLFW, Jolt Physics, irrKlang, ImGui");
        ImGui::Separator();
        ImGui::Text("Copyright (c) 2026. All rights reserved.");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();  // DockSpace

    m_ImGuiLayer.EndFrame();
}

void EditorSystem::DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                auto& sm = ServiceLocator::Instance().Require<SceneManager>();
                auto& globalScene = ServiceLocator::Instance().Require<Scene>();
                auto* resourceManager = ServiceLocator::Instance().Resolve<ResourceManager>();
                if (resourceManager)
                {
                    auto scenes = sm.GetAllScenes();
                    for (const auto& sceneRecord : scenes)
                    {
                        if (!sceneRecord.filePath.empty())
                        {
                            SceneSerializer::Serialize(sceneRecord.filePath, globalScene, *resourceManager,
                                                       sceneRecord.name);
                            LOGGER_INFO("EditorSystem")
                                << "Saved scene: " << sceneRecord.name << " to " << sceneRecord.filePath;
                        }
                    }
                }
            }
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                auto core = ServiceLocator::Instance().Resolve<RuntimeCore>();
                if (core)
                    core->GetEngineLoop().Stop();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            for (auto& panel : m_Panels)
            {
                if (panel->GetGroup() == PanelGroup::Scene)
                {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open))
                        panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            for (auto& panel : m_Panels)
            {
                if (panel->GetGroup() == PanelGroup::Debug || panel->GetGroup() == PanelGroup::Tools)
                {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open))
                        panel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            for (auto& panel : m_Panels)
            {
                if (panel->GetGroup() == PanelGroup::Help)
                {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open))
                        panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About AxisEngine"))
            {
                m_ShowAboutPopup = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

static std::vector<std::string> s_UndoStack;
static std::vector<std::string> s_RedoStack;

static std::string CaptureEditorSceneState(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    if (!rm)
        return {};

    const std::string tempFile = "temp_undo.axs";
    if (!SceneSerializer::Serialize(tempFile, scene, *rm))
        return {};

    std::ifstream f(tempFile, std::ios::binary);
    if (!f.is_open())
        return {};

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    std::error_code ec;
    std::filesystem::remove(tempFile, ec);
    return content;
}

static void RestoreEditorSceneState(Scene& scene, const std::string& state)
{
    if (state.empty())
        return;

    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto* audio = sl.Resolve<AudioService>();
    auto* sceneMgr = sl.Resolve<SceneManager>();
    if (!rm)
        return;

    const std::string tempRestoreFile = "temp_restore.axs";
    std::ofstream f(tempRestoreFile, std::ios::binary);
    if (!f.is_open())
        return;

    f << state;
    f.close();

    scene.registry.clear();
    SceneSerializer::Deserialize(tempRestoreFile, scene, *rm, phys, audio);
    if (sceneMgr)
        sceneMgr->RebuildEntityRecords(scene);

    std::error_code ec;
    std::filesystem::remove(tempRestoreFile, ec);

    SceneHierarchyPanel::SetSelectedEntity(entt::null);
    EventManager::Instance().Publish(SceneChangedEvent{&scene.registry, &scene});
}

void EditorSystem::PushUndoState(Scene& scene)
{
    std::string content = CaptureEditorSceneState(scene);
    if (content.empty() || (!s_UndoStack.empty() && s_UndoStack.back() == content))
        return;

    s_UndoStack.push_back(content);
    s_RedoStack.clear();
    if (s_UndoStack.size() > 50)
        s_UndoStack.erase(s_UndoStack.begin());
}

void EditorSystem::PerformUndo(Scene& scene)
{
    if (s_UndoStack.empty())
        return;

    std::string currentState = CaptureEditorSceneState(scene);
    if (currentState.empty())
        return;

    if (s_UndoStack.back() != currentState)
    {
        s_RedoStack.push_back(currentState);
        RestoreEditorSceneState(scene, s_UndoStack.back());
        return;
    }

    if (s_UndoStack.size() < 2)
        return;

    s_RedoStack.push_back(currentState);
    s_UndoStack.pop_back();
    RestoreEditorSceneState(scene, s_UndoStack.back());
}

void EditorSystem::PerformRedo(Scene& scene)
{
    if (s_RedoStack.empty())
        return;

    std::string redoState = s_RedoStack.back();
    s_RedoStack.pop_back();

    std::string currentState = CaptureEditorSceneState(scene);
    if (!currentState.empty() && (s_UndoStack.empty() || s_UndoStack.back() != currentState))
        s_UndoStack.push_back(currentState);

    if (s_UndoStack.empty() || s_UndoStack.back() != redoState)
        s_UndoStack.push_back(redoState);
    if (s_UndoStack.size() > 50)
        s_UndoStack.erase(s_UndoStack.begin(), s_UndoStack.begin() + (s_UndoStack.size() - 50));

    RestoreEditorSceneState(scene, redoState);
}
#endif
