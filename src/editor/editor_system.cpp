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
#include <platform/interface/i_ui_input_capture.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/binary_scene_serializer.h>
#include <scene/type/scene_events.h>
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>
#include <algorithm>
#include <atomic>
#include <chrono>
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
#include <editor/panels/animation_graph_panel.h>
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
#include <editor/panels/vfx_graph_panel.h>

namespace
{
std::vector<std::string> s_UndoStack;
std::vector<std::string> s_RedoStack;

std::filesystem::path MakeEditorTempPath(const char* purpose)
{
    static std::atomic_uint64_t counter{0};
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("axis_editor_" + std::string(purpose) + "_" + std::to_string(timestamp) + "_" +
            std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) + ".axs");
}
}  // namespace

EditorSystem::EditorSystem() = default;
EditorSystem::~EditorSystem() = default;

void EditorSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IEditorExtensionRegistry>(this);
    sl.Register<IUIInputCapture>(&m_ImGuiLayer);
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

    LOGGER_INFO("EditorSystem") << "Initializing built-in editor modules...";

    auto generalModule = std::make_unique<GeneralEditorModule>();
    generalModule->Initialize();
    m_Modules.push_back(std::move(generalModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "General", EditorExtensionKind::Module});

    auto renderModule = std::make_unique<RenderEditorModule>();
    renderModule->Initialize();
    m_Modules.push_back(std::move(renderModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "Render", EditorExtensionKind::Module});

    auto physicsModule = std::make_unique<PhysicsEditorModule>();
    physicsModule->Initialize();
    m_Modules.push_back(std::move(physicsModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "Physics", EditorExtensionKind::Module});

    auto gizmoModule = std::make_unique<GizmoEditorModule>();
    gizmoModule->Initialize();
    gizmoModule->SetSharedResources(debugFont, textShader, textQuad);
    m_Modules.push_back(std::move(gizmoModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "Gizmo", EditorExtensionKind::Module});

    auto cameraModule = std::make_unique<CameraEditorModule>();
    cameraModule->Initialize();
    m_Modules.push_back(std::move(cameraModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "Camera", EditorExtensionKind::Module});

    // Wait, let's setup ImGui
    auto* ioHandler = sl.Resolve<IOHandler>();
    if (ioHandler)
    {
        auto* window = ioHandler->GetMonitorManager().GetWindow();
        auto* graphicsContext = sl.Resolve<IGraphicsContext>();
        if (window && graphicsContext)
        {
            m_ImGuiLayer.Initialize(*window, *graphicsContext);
        }
    }

    // Register Panels
    m_Panels.push_back(std::make_unique<SceneHierarchyPanel>());
    m_Panels.push_back(std::make_unique<ResourceBrowserPanel>());
    m_Panels.push_back(std::make_unique<FileHierarchyPanel>());
    m_Panels.push_back(std::make_unique<ToolsPanel>());
    m_Panels.push_back(std::make_unique<SettingsPanel>());
    m_Panels.push_back(std::make_unique<ProfilerPanel>());
    m_Panels.push_back(std::make_unique<ConsolePanel>());
    m_Panels.push_back(std::make_unique<StatePanel>());
    m_Panels.push_back(std::make_unique<NetworkPanel>());
    m_Panels.push_back(std::make_unique<HelpPanel>());
    m_Panels.push_back(std::make_unique<AnimationGraphPanel>());
    m_Panels.push_back(std::make_unique<VFXGraphPanel>());
    m_PanelOwners.assign(m_Panels.size(), "axis.editor");

    const char* defaultPanelNames[] = {"Scene Hierarchy", "Resource Browser", "File Hierarchy", "Tools",
                                       "Settings", "Profiler", "Console", "State", "Network", "Help",
                                       "Animation Graph", "VFX Graph"};
    for (const char* name : defaultPanelNames)
        m_Extensions.push_back({"axis.editor", name, EditorExtensionKind::Panel});

    for (auto& panel : m_Panels)
    {
        panel->Initialize();
    }
    m_Initialized = true;

    LOGGER_INFO("EditorSystem") << "Initialized completely.";

    // Wire log callback to console panel
    LogManager::Instance().SetLogCallback([](LogType type, const std::string& tag, const std::string& msg) {
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
    LogManager::Instance().SetLogCallback({});
    for (auto it = m_Panels.rbegin(); it != m_Panels.rend(); ++it) (*it)->Shutdown();
    for (auto it = m_Modules.rbegin(); it != m_Modules.rend(); ++it) (*it)->Shutdown();
    m_Modules.clear();
    m_PanelOwners.clear();
    m_ModuleOwners.clear();
    m_Extensions.clear();
    m_Initialized = false;
    m_Panels.clear();
    m_ImGuiLayer.Shutdown();
    s_UndoStack.clear();
    s_RedoStack.clear();
    m_NumberKeyPressed.fill(false);
    m_NudgeTimer = 0.0f;
}

void EditorSystem::OnUpdate(float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto* ioHandler = sl.Resolve<IOHandler>();
    if (!ioHandler)
        return;

    KeyboardManager& kb = ioHandler->GetKeyboard();
    // Built-in editor module shortcuts.
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
        panel->OnUpdate(dt);
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
                    if (rec.filePath.ends_with(".axsb"))
                    {
                        BinarySceneSerializer serializer;
                        serializer.Serialize(rec.filePath, globalScene);
                    }
                    else
                    {
                        auto* phys = sl.Resolve<IPhysicsWorld>();
                        auto* audio = sl.Resolve<AudioService>();
                        SceneSerializer serializer(*resourceManager, phys, audio);
                        serializer.Serialize(rec.filePath, globalScene, rec.name);
                    }
                    LOGGER_INFO("EditorSystem") << "Saved scene: " << rec.name;
                }
            }
        }
    }
    if (!kb.GetKey(Key::S))
        m_CtrlSPressed = false;

    // Ctrl+Z — Undo
    if (ctrl && kb.GetKey(Key::Z) && !m_ZPressed)
    {
        m_ZPressed = true;
        auto& globalScene = sl.Require<Scene>();
        if (shift)
            PerformRedo(globalScene);
        else
            PerformUndo(globalScene);
    }
    if (!kb.GetKey(Key::Z))
        m_ZPressed = false;

    if (ctrl && kb.GetKey(Key::Y) && !m_YPressed)
    {
        m_YPressed = true;
        auto& globalScene = sl.Require<Scene>();
        PerformRedo(globalScene);
    }
    if (!kb.GetKey(Key::Y))
        m_YPressed = false;

    // Ctrl+D — Duplicate selected entity
    // (handled in SceneHierarchyPanel via shared selection)

    // --- Nudge Tool ---
    m_NudgeTimer -= dt;
    bool alt = kb.GetKey(Key::LeftAlt) || kb.GetKey(Key::RightAlt);
    if (alt && SceneHierarchyPanel::s_SelectedEntity != entt::null && m_NudgeTimer <= 0.0f)
    {
        auto& globalScene = sl.Require<Scene>();
        auto& reg = globalScene.GetRegistry();
        if (reg.valid(SceneHierarchyPanel::s_SelectedEntity))
        {
            auto cm = sl.Resolve<ConfigManager>();
            if (cm)
            {
                auto conf = cm->GetConfig();
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
                    m_NudgeTimer = 0.15f;
                    globalScene.MarkTransformDirty(SceneHierarchyPanel::s_SelectedEntity);
                }
            }
        }
    }

    // --- Debug & System Toggles ---
    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (cm)
    {
        auto conf = cm->GetConfig();
        bool changed = false;

        if (kb.GetKey(Key::F1) && !m_F1Pressed)
        {
            m_F1Pressed = true;
            conf.debug.entityNames = !conf.debug.entityNames;
            changed = true;
        }
        else if (!kb.GetKey(Key::F1))
            m_F1Pressed = false;

        if (kb.GetKey(Key::F2) && !m_F2Pressed)
        {
            m_F2Pressed = true;
            conf.debug.gizmos = !conf.debug.gizmos;
            changed = true;
        }
        else if (!kb.GetKey(Key::F2))
            m_F2Pressed = false;

        if (kb.GetKey(Key::F3) && !m_F3Pressed)
        {
            m_F3Pressed = true;
            conf.debug.lightGizmos = !conf.debug.lightGizmos;
            changed = true;
        }
        else if (!kb.GetKey(Key::F3))
            m_F3Pressed = false;

        if (kb.GetKey(Key::G))
        {
            if (!m_GPressed)
            {
                m_GPressed = true;
                if (ctrl)
                {
                    conf.debug.gridSnapEnabled = !conf.debug.gridSnapEnabled;
                    changed = true;
                }
            }
        }
        else if (!kb.GetKey(Key::G))
            m_GPressed = false;

        if (ctrl && kb.GetKey(Key::H) && !m_HPressed)
        {
            m_HPressed = true;
            conf.debug.gridIndicatorEnabled = !conf.debug.gridIndicatorEnabled;
            changed = true;
        }
        else if (!kb.GetKey(Key::H))
            m_HPressed = false;

        if (changed)
            cm->UpdateConfig(conf, ConfigChangedEvent::Debug);

        // Reload Active Scene (Ctrl+R)
        if (ctrl && kb.GetKey(Key::R) && !m_RPressed)
        {
            m_RPressed = true;
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
            m_RPressed = false;
        }

        // Toggle Audio System (F4)
        if (kb.GetKey(Key::F4) && !m_F4Pressed)
        {
            m_F4Pressed = true;
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
            m_F4Pressed = false;

        // Toggle Post Process (F5)
        if (kb.GetKey(Key::F5) && !m_F5Pressed)
        {
            m_F5Pressed = true;
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
            m_F5Pressed = false;

        // Toggle Force Free Cursor (F6)
        if (kb.GetKey(Key::F6) && !shift && !m_F6Pressed)
        {
            m_F6Pressed = true;
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
            m_F6Pressed = false;
    }

    // Panel toggle shortcuts. Ctrl+1..0 addresses the first bank; Ctrl+Shift+1..0 the second.
    Key numKeys[] = {Key::_1, Key::_2, Key::_3, Key::_4, Key::_5, Key::_6, Key::_7, Key::_8, Key::_9, Key::_0};

    if (ctrl)
    {
        const int panelOffset = shift ? 10 : 0;
        const int shortcutCount = std::min(std::max(static_cast<int>(m_Panels.size()) - panelOffset, 0), 10);
        for (int i = 0; i < shortcutCount; ++i)
        {
            if (kb.GetKey(numKeys[i]) && !m_NumberKeyPressed[i])
            {
                m_NumberKeyPressed[i] = true;
                auto& panel = m_Panels[panelOffset + i];
                panel->SetOpen(!panel->IsOpen());
            }
            else if (!kb.GetKey(numKeys[i]))
            {
                m_NumberKeyPressed[i] = false;
            }
        }
    }
    else
    {
        m_NumberKeyPressed.fill(false);
    }
}

void EditorSystem::Render(Scene& scene)
{
    // Modules may contribute overlays to the main 3D scene.
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

    // Panels and module UI share the ImGui pass.
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
        ImGui::Text("Tech Stack: EnTT, OpenGL 3.3+, GLFW, Bullet Physics, irrKlang, ImGui");
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
                            if (sceneRecord.filePath.ends_with(".axsb"))
                            {
                                BinarySceneSerializer serializer;
                                serializer.Serialize(sceneRecord.filePath, globalScene);
                            }
                            else
                            {
                                auto* phys = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
                                auto* audio = ServiceLocator::Instance().Resolve<AudioService>();
                                SceneSerializer serializer(*resourceManager, phys, audio);
                                serializer.Serialize(sceneRecord.filePath, globalScene, sceneRecord.name);
                            }
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
            for (size_t panelIndex = 0; panelIndex < m_Panels.size(); ++panelIndex)
            {
                auto& panel = m_Panels[panelIndex];
                if (panel->GetGroup() == PanelGroup::Scene)
                {
                    bool open = panel->IsOpen();
                    const int keyIndex = static_cast<int>(panelIndex % 10);
                    const char keyName = keyIndex == 9 ? '0' : static_cast<char>('1' + keyIndex);
                    const std::string shortcut = panelIndex < 10
                                                     ? std::string("Ctrl+") + keyName
                                                     : panelIndex < 20 ? std::string("Ctrl+Shift+") + keyName : "";
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), &open))
                        panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            for (size_t panelIndex = 0; panelIndex < m_Panels.size(); ++panelIndex)
            {
                auto& panel = m_Panels[panelIndex];
                if (panel->GetGroup() == PanelGroup::Debug || panel->GetGroup() == PanelGroup::Tools)
                {
                    bool open = panel->IsOpen();
                    const int keyIndex = static_cast<int>(panelIndex % 10);
                    const char keyName = keyIndex == 9 ? '0' : static_cast<char>('1' + keyIndex);
                    const std::string shortcut = panelIndex < 10
                                                     ? std::string("Ctrl+") + keyName
                                                     : panelIndex < 20 ? std::string("Ctrl+Shift+") + keyName : "";
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), &open))
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

static std::string CaptureEditorSceneState(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    if (!rm)
        return {};

    const auto tempFile = MakeEditorTempPath("undo");
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto* audio = sl.Resolve<AudioService>();
    SceneSerializer serializer(*rm, phys, audio);
    if (!serializer.Serialize(tempFile.string(), scene))
        return {};

    std::ifstream f(tempFile, std::ios::binary);
    if (!f.is_open())
    {
        std::error_code ec;
        std::filesystem::remove(tempFile, ec);
        return {};
    }

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

    const auto tempRestoreFile = MakeEditorTempPath("restore");
    std::ofstream f(tempRestoreFile, std::ios::binary);
    if (!f.is_open())
        return;

    f << state;
    f.close();

    scene.GetRegistry().clear();
    SceneSerializer serializer(*rm, phys, audio);
    serializer.Deserialize(tempRestoreFile.string(), scene);
    if (sceneMgr)
        sceneMgr->RebuildEntityRecords(scene);

    std::error_code ec;
    std::filesystem::remove(tempRestoreFile, ec);

    SceneHierarchyPanel::SetSelectedEntity(entt::null);
    EventManager::Instance().Publish(SceneChangedEvent{&scene.GetRegistry(), &scene});
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

bool EditorSystem::RegisterModule(std::string owner, std::string name, ModuleFactory factory)
{
    if (owner.empty() || name.empty() || !factory)
        return false;
    if (std::any_of(m_Extensions.begin(), m_Extensions.end(), [&](const auto& extension) {
            return extension.owner == owner && extension.name == name && extension.kind == EditorExtensionKind::Module;
        }))
        return false;
    auto module = factory();
    if (!module)
        return false;
    if (m_Initialized)
        module->Initialize();
    m_Modules.push_back(std::move(module));
    m_ModuleOwners.push_back(owner);
    m_Extensions.push_back({std::move(owner), std::move(name), EditorExtensionKind::Module});
    return true;
}

bool EditorSystem::RegisterPanel(std::string owner, std::string name, PanelFactory factory)
{
    if (owner.empty() || name.empty() || !factory)
        return false;
    if (std::any_of(m_Extensions.begin(), m_Extensions.end(), [&](const auto& extension) {
            return extension.owner == owner && extension.name == name && extension.kind == EditorExtensionKind::Panel;
        }))
        return false;
    auto panel = factory();
    if (!panel)
        return false;
    if (m_Initialized)
        panel->Initialize();
    m_Panels.push_back(std::move(panel));
    m_PanelOwners.push_back(owner);
    m_Extensions.push_back({std::move(owner), std::move(name), EditorExtensionKind::Panel});
    return true;
}

size_t EditorSystem::UnregisterOwner(std::string_view owner)
{
    size_t removed = 0;
    for (size_t index = m_Modules.size(); index-- > 0;)
    {
        if (m_ModuleOwners[index] != owner)
            continue;
        if (m_Initialized)
            m_Modules[index]->Shutdown();
        m_Modules.erase(m_Modules.begin() + static_cast<std::ptrdiff_t>(index));
        m_ModuleOwners.erase(m_ModuleOwners.begin() + static_cast<std::ptrdiff_t>(index));
        ++removed;
    }
    for (size_t index = m_Panels.size(); index-- > 0;)
    {
        if (m_PanelOwners[index] != owner)
            continue;
        if (m_Initialized)
            m_Panels[index]->Shutdown();
        m_Panels.erase(m_Panels.begin() + static_cast<std::ptrdiff_t>(index));
        m_PanelOwners.erase(m_PanelOwners.begin() + static_cast<std::ptrdiff_t>(index));
        ++removed;
    }
    std::erase_if(m_Extensions, [owner](const auto& extension) { return extension.owner == owner; });
    return removed;
}

std::vector<EditorExtensionInfo> EditorSystem::GetExtensions() const
{
    auto extensions = m_Extensions;
    std::sort(extensions.begin(), extensions.end(), [](const auto& left, const auto& right) {
        if (left.owner != right.owner)
            return left.owner < right.owner;
        if (left.kind != right.kind)
            return left.kind < right.kind;
        return left.name < right.name;
    });
    return extensions;
}
#endif
