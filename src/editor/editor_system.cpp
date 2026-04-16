#include <editor/editor_system.h>
#include <ecs/logic/debug/i_debug_module.h>
#include <ecs/logic/debug/modules/general_debug_module.h>
#include <ecs/logic/debug/modules/overlay_debug_module.h>
#include <ecs/logic/debug/modules/render_debug_module.h>
#include <ecs/logic/debug/modules/physics_debug_module.h>
#include <ecs/logic/debug/modules/gizmo_debug_module.h>
#include <ecs/logic/debug/modules/camera_debug_module.h>
#include <ecs/logic/debug/modules/shadow_debug_module.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>

#ifdef ENABLE_EDITOR

#include <platform/logic/io_handler.h>
#include <core/logic/logger.h>
#include <scene/logic/scene.h>
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
#include <editor/panels/code_editor_panel.h>

EditorSystem::EditorSystem() {}
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

    auto generalModule = std::make_unique<GeneralDebugModule>();
    generalModule->Initialize();
    m_Modules.push_back(std::move(generalModule));

    auto overlayModule = std::make_unique<OverlayDebugModule>();
    overlayModule->Initialize();
    overlayModule->SetSharedResources(debugFont, textShader, textQuad);
    m_Modules.push_back(std::move(overlayModule));

    auto renderModule = std::make_unique<RenderDebugModule>();
    renderModule->Initialize();
    m_Modules.push_back(std::move(renderModule));

    auto physicsModule = std::make_unique<PhysicsDebugModule>();
    physicsModule->Initialize();
    m_Modules.push_back(std::move(physicsModule));

    auto gizmoModule = std::make_unique<GizmoDebugModule>();
    gizmoModule->Initialize();
    gizmoModule->SetSharedResources(debugFont, textShader, textQuad);
    m_Modules.push_back(std::move(gizmoModule));

    auto cameraModule = std::make_unique<CameraDebugModule>();
    cameraModule->Initialize();
    m_Modules.push_back(std::move(cameraModule));

    auto shadowModule = std::make_unique<ShadowDebugModule>();
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
    m_Panels.push_back(std::make_unique<CodeEditorPanel>());

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
    m_ImGuiLayer.EndFrame();
}

void EditorSystem::DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            for (auto& panel : m_Panels) {
                bool open = panel->IsOpen();
                if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &open)) {
                    panel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}
#endif
