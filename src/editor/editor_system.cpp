#include <editor/editor_system.h>
#include <editor/i_editor_module.h>
#include <editor/modules/general_editor_module.h>
#include <editor/modules/render_editor_module.h>
#include <editor/modules/physics_editor_module.h>
#include <editor/modules/gizmo_editor_module.h>
#include <editor/modules/camera_editor_module.h>
#include <editor/modules/shadow_editor_module.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>

#ifdef ENABLE_EDITOR

#include <platform/logic/io_handler.h>
#include <core/logic/logger.h>
#include <core/app/runtime_core.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <filesystem>
#include <map>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <platform/logic/monitor_manager.h>
#include <algorithm>
#include <imgui.h>

// Panel includes
#include <editor/panels/scene_hierarchy_panel.h>
#include <editor/panels/resource_browser_panel.h>
#include <editor/panels/stats_panel.h>
#include <editor/panels/tools_panel.h>
#include <editor/panels/settings_panel.h>
#include <editor/panels/file_hierarchy_panel.h>
#include <editor/panels/help_panel.h>

EditorSystem::EditorSystem() : m_PreHoverCursorMode(CursorMode::Normal) {}
EditorSystem::~EditorSystem() {}

REGISTER_SYSTEM(EditorSystem)

void EditorSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<EditorSystem>(this);
    auto* res = sl.Resolve<ResourceManager>();
    if (!res) {
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

    auto shadowModule = std::make_unique<ShadowEditorModule>();
    shadowModule->Initialize();
    m_Modules.push_back(std::move(shadowModule));

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
    m_Panels.push_back(std::make_unique<StatsPanel>());
    m_Panels.push_back(std::make_unique<ToolsPanel>());
    m_Panels.push_back(std::make_unique<SettingsPanel>());
    m_Panels.push_back(std::make_unique<FileHierarchyPanel>());
    m_Panels.push_back(std::make_unique<HelpPanel>());

    for (auto& panel : m_Panels) {
        panel->Initialize();
    }

    LOGGER_INFO("EditorSystem") << "Initialized completely.";
}

void EditorSystem::Shutdown()
{
    m_ImGuiLayer.Shutdown();
}

void EditorSystem::OnUpdate(float dt)
{
    auto* ioHandler = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!ioHandler) return;

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

    for (auto& panel : m_Panels) {
        if (panel->IsOpen()) {
            panel->OnUpdate(dt);
        }
    }

    // Auto Reload Scene Feature
    static float reloadTimer = 0.0f;
    reloadTimer += dt;
    if (reloadTimer >= 1.0f) {
        reloadTimer = 0.0f;
        auto& sm = ServiceLocator::Instance().Require<SceneManager>();
        static std::map<std::string, std::filesystem::file_time_type> fileWrites;
        for (const auto& rec : sm.GetAllScenes()) {
            if (!rec.filePath.empty() && std::filesystem::exists(rec.filePath)) {
                auto lwT = std::filesystem::last_write_time(rec.filePath);
                if (fileWrites.find(rec.filePath) != fileWrites.end()) {
                    if (lwT > fileWrites[rec.filePath]) {
                        LOGGER_INFO("Editor") << "Auto-Reloading modified scene: " << rec.filePath;
                        sm.QueueChangeScene(rec.filePath);
                    }
                }
                fileWrites[rec.filePath] = lwT;
            }
        }
    }

    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>()) {
        auto mode = io->GetMouse().GetCursorMode();
        if (mode == CursorMode::Hidden || mode == CursorMode::Locked || mode == CursorMode::LockedHidden) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        } else {
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        }
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
    if (!m_ImGuiLayer.IsInitialized()) return;

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

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::Begin("AxisEngine_DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("AxisDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    DrawMenuBar();

    for (auto& panel : m_Panels) {
        if (panel->IsOpen()) {
            panel->OnImGui(scene);
        }
    }

    ImGui::End(); // DockSpace

    // Auto-switch cursor to Disabled when hovering any editor panel
    auto* ioHandler = ServiceLocator::Instance().Resolve<IOHandler>();
    if (ioHandler) {
        auto& mouse = ioHandler->GetMouse();
        bool hoveringPanel = ImGui::GetIO().WantCaptureMouse;
        CursorMode mode = mouse.GetCursorMode();
        
        if (hoveringPanel && !m_WasHoveringPanel) {
            // Just entered panel
            m_PreHoverCursorMode = mode;
            if (mode != CursorMode::Disabled) {
                mouse.SetCursorMode(CursorMode::Disabled);
            }
            m_WasHoveringPanel = true;
        } else if (!hoveringPanel && m_WasHoveringPanel) {
            // Just exited panel
            if (mode == CursorMode::Disabled) {
                mouse.SetCursorMode(m_PreHoverCursorMode);
            }
            m_WasHoveringPanel = false;
        }
    }

    m_ImGuiLayer.EndFrame();
}

void EditorSystem::DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) {
                auto core = ServiceLocator::Instance().Resolve<RuntimeCore>();
                if (core) core->GetEngineLoop().Stop();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            for (auto& panel : m_Panels) {
                if (panel->GetTitle() == "Scene Hierarchy" || panel->GetTitle() == "Resource Browser" || panel->GetTitle() == "File Hierarchy") {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open)) panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            for (auto& panel : m_Panels) {
                if (panel->GetTitle() == "Stats" || panel->GetTitle() == "Settings" || panel->GetTitle() == "Tools") {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open)) panel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            for (auto& panel : m_Panels) {
                if (panel->GetTitle() == "Help") {
                    bool open = panel->IsOpen();
                    if (ImGui::MenuItem("Debug Controls", nullptr, &open)) panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About AxisEngine")) {
                ImGui::OpenPopup("About AxisEngine");
            }
            ImGui::EndMenu();
        }
        
        // About Modal
        if (ImGui::BeginPopupModal("About AxisEngine", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("AxisEngine - Advanced 5-Layer ECS Engine");
            ImGui::Separator();
            ImGui::Text("Developed by Duong.");
            ImGui::Text("Build: Debug/Dev");
            if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::EndMenuBar();
    }
}
#endif
