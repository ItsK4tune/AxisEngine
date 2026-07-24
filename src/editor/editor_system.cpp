#include <editor/editor_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <editor/i_editor_module.h>
#include <editor/editor_shortcut.h>
#include <editor/modules/camera_editor_module.h>
#include <editor/modules/diagnostics_editor_module.h>
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
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/type/shader_abi.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/binary_scene_serializer.h>
#include <scene/type/scene_events.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <map>
#include <unordered_set>

// Panel includes
#include <core/logic/event_manager.h>
#include <core/logic/time_service.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <ecs/interface/i_geometry_service.h>
#include <editor/panels/console_panel.h>
#include <editor/panels/animation_graph_panel.h>
#include <editor/panels/file_hierarchy_panel.h>
#include <editor/panels/frame_debugger_panel.h>
#include <editor/panels/help_panel.h>
#include <editor/panels/inspector_panel.h>
#include <editor/panels/input_actions_panel.h>
#include <editor/panels/lighting_panel.h>
#include <editor/panels/navigation_panel.h>
#include <editor/panels/profiler_panel.h>
#include <editor/panels/prefab_panel.h>
#include <editor/panels/project_assets_panel.h>
#include <editor/panels/resource_browser_panel.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <editor/panels/settings_panel.h>
#include <editor/panels/state_panel.h>
#include <editor/panels/network_panel.h>
#include <editor/panels/tools_panel.h>
#include <editor/panels/vfx_graph_panel.h>
#include <render/interface/i_post_process_registry.h>
#include <ecs/logic/post_process_system.h>
#include <limits>
#include <iterator>

namespace
{
constexpr const char* kSelectionOutlineOwner = "axis.editor.selection";
constexpr const char* kSelectionOutlineShader = "editor_selection_outline";
constexpr const char* kDockspaceName = "AxisDockSpace_v4";
constexpr const char* kSceneHierarchyTitle = "Scene Hierarchy [Ctrl+1]";
constexpr const char* kProjectAssetsTitle = "Project / Assets [Ctrl+2]";
constexpr const char* kInspectorTitle = "Inspector [Ctrl+3]";
constexpr const char* kConsoleTitle = "Console [Ctrl+7]";

std::string CapturePersistedSceneState(Scene& scene);

void BuildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, size);

    ImGuiID center = dockspaceId;
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, nullptr, &center);
    ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.38f, nullptr, &left);

    ImGui::DockBuilderDockWindow(kSceneHierarchyTitle, left);
    ImGui::DockBuilderDockWindow(kProjectAssetsTitle, leftBottom);
    ImGui::DockBuilderDockWindow(kInspectorTitle, right);
    ImGui::DockBuilderDockWindow(kConsoleTitle, bottom);
    ImGui::DockBuilderFinish(dockspaceId);
}

entt::entity ResolveRenderedEntity(Scene& scene, uint32_t renderedId)
{
    if (renderedId == std::numeric_limits<uint32_t>::max())
        return entt::null;

    auto entities = scene.View<InfoComponent>();
    for (auto entity : entities)
    {
        if (static_cast<uint32_t>(entt::to_entity(entity)) == renderedId)
            return entity;
    }
    return entt::null;
}

}  // namespace

EditorSystem::EditorSystem() = default;
EditorSystem::~EditorSystem() = default;

void EditorSystem::EnsureSelectionOutlineEffect()
{
    if (m_SelectionOutlineHandle != 0)
        return;

    auto* registry = ServiceLocator::Instance().Resolve<IPostProcessRegistry>();
    if (!registry)
        return;

    for (const auto& effect : registry->GetRegisteredEffects())
    {
        if (effect.descriptor.owner == kSelectionOutlineOwner &&
            effect.descriptor.name == "Selection Outline")
        {
            m_SelectionOutlineHandle = effect.handle;
            return;
        }
    }

    PostProcessEffectDescriptor descriptor;
    descriptor.owner = kSelectionOutlineOwner;
    descriptor.name = "Selection Outline";
    descriptor.shaderName = kSelectionOutlineShader;
    descriptor.priority = 100;
    descriptor.affectUI = false;
    descriptor.inputs = PostProcessInput::Color | PostProcessInput::Depth | PostProcessInput::EntityId;
    m_SelectionOutlineHandle = registry->RegisterEffect(std::move(descriptor));
}

void EditorSystem::SetEnabled(bool enabled)
{
    if (m_Enabled == enabled)
        return;

    if (!enabled)
    {
        if (auto* postProcess = ServiceLocator::Instance().Resolve<PostProcessSystem>())
            postProcess->SetPresentToBackbuffer(true);
        if (auto* scene = ServiceLocator::Instance().Resolve<Scene>())
            m_TransformGizmo.CommitDrag(*scene);
        else
            m_TransformGizmo.CancelDrag();
        if (m_RunMode != EditorRunMode::Edit)
        {
            if (auto* scene = ServiceLocator::Instance().Resolve<Scene>())
                StopPlayMode(*scene);
        }
        m_BoxSelecting = false;
        if (m_SelectionOutlineHandle != 0)
        {
            if (auto* postProcess = ServiceLocator::Instance().Resolve<IPostProcessRegistry>())
                postProcess->UnregisterEffect(m_SelectionOutlineHandle);
        }
        m_SelectionOutlineHandle = 0;

        m_ModuleEnabledBeforeDisable.clear();
        m_ModuleEnabledBeforeDisable.reserve(m_Modules.size());
        for (auto& module : m_Modules)
        {
            m_ModuleEnabledBeforeDisable.push_back(module->IsEnabled());
            module->SetEnabled(false);
        }
        m_ImGuiLayer.SetPointerInputEnabled(false);
        if (auto* ioHandler = ServiceLocator::Instance().Resolve<IOHandler>())
            ioHandler->GetMouse().ExitEditorMode();
    }
    else
    {
        if (auto* postProcess = ServiceLocator::Instance().Resolve<PostProcessSystem>())
            postProcess->SetPresentToBackbuffer(true);
        const size_t restoreCount = std::min(m_Modules.size(), m_ModuleEnabledBeforeDisable.size());
        for (size_t index = 0; index < restoreCount; ++index)
            m_Modules[index]->SetEnabled(m_ModuleEnabledBeforeDisable[index]);
        m_ModuleEnabledBeforeDisable.clear();
    }
    m_Enabled = enabled;
}

void EditorSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<EditorSelection>(&m_Selection);
    sl.Register<EditorCommandHistory>(&m_CommandHistory);
    sl.Register<IEditorExtensionRegistry>(this);
    sl.Register<IKeyboardInputRouter>(this);
    sl.Register<IUIInputCapture>(&m_ImGuiLayer);
    sl.Register<EditorViewportState>(&m_ViewportState);
    if (auto* postProcess = sl.Resolve<PostProcessSystem>())
        postProcess->SetPresentToBackbuffer(true);
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

    auto diagnosticsModule = std::make_unique<DiagnosticsEditorModule>();
    diagnosticsModule->Initialize();
    m_Modules.push_back(std::move(diagnosticsModule));
    m_ModuleOwners.push_back("axis.editor");
    m_Extensions.push_back({"axis.editor", "Diagnostics", EditorExtensionKind::Module});

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

    m_Panels.push_back(std::make_unique<SceneHierarchyPanel>());
    m_Panels.push_back(std::make_unique<ProjectAssetsPanel>());
    m_Panels.push_back(std::make_unique<ToolsPanel>());
    m_Panels.push_back(std::make_unique<SettingsPanel>());
    m_Panels.push_back(std::make_unique<ProfilerPanel>());
    m_Panels.push_back(std::make_unique<ConsolePanel>());
    m_Panels.push_back(std::make_unique<StatePanel>());
    m_Panels.push_back(std::make_unique<NetworkPanel>());
    m_Panels.push_back(std::make_unique<HelpPanel>());
    m_Panels.push_back(std::make_unique<AnimationGraphPanel>());
    m_Panels.push_back(std::make_unique<VFXGraphPanel>());
    m_Panels.push_back(std::make_unique<InspectorPanel>());
    m_Panels.push_back(std::make_unique<InputActionsPanel>());
    m_Panels.push_back(std::make_unique<NavigationPanel>());
    m_Panels.push_back(std::make_unique<FrameDebuggerPanel>());
    m_Panels.push_back(std::make_unique<LightingPanel>());
    m_Panels.push_back(std::make_unique<PrefabPanel>());
    m_PanelOwners.assign(m_Panels.size(), "axis.editor");
    m_PanelShortcutSlots = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 12, 13, 14, 15, 16};

    const char* defaultPanelNames[] = {"Scene Hierarchy", "Project / Assets", "Tools",
                                       "Settings", "Profiler", "Console", "State", "Network", "Help",
                                       "Animation Graph", "VFX Graph", "Inspector", "Input Actions",
                                       "Navigation", "Frame Debugger", "Lighting", "Prefabs"};
    for (const char* name : defaultPanelNames)
        m_Extensions.push_back({"axis.editor", name, EditorExtensionKind::Panel});

    for (auto& panel : m_Panels)
    {
        panel->Initialize();
    }
    m_Initialized = true;

    LOGGER_INFO("EditorSystem") << "Initialized completely.";

    LogManager::Instance().SetLogCallback([](LogType type, const std::string& tag, const std::string& msg) {
        if (ConsolePanel::s_Instance)
        {
            ConsolePanel::s_Instance->PushLog(type, tag, msg);
        }
    });
}

void EditorSystem::Shutdown()
{
    if (auto* postProcess = ServiceLocator::Instance().Resolve<PostProcessSystem>())
        postProcess->SetPresentToBackbuffer(true);
    if (m_SelectionBuffer != 0)
    {
        if (auto* graphics = ServiceLocator::Instance().Resolve<IGraphicsContext>())
            graphics->GetBufferManager().DeleteBuffer(m_SelectionBuffer);
        m_SelectionBuffer = 0;
    }
    m_LastOutlinedEntityIds.clear();
    if (m_RunMode != EditorRunMode::Edit)
    {
        if (auto* scene = ServiceLocator::Instance().Resolve<Scene>())
            StopPlayMode(*scene);
    }
    if (auto* postProcess = ServiceLocator::Instance().Resolve<IPostProcessRegistry>())
        postProcess->UnregisterOwner(kSelectionOutlineOwner);
    m_SelectionOutlineHandle = 0;
    if (auto* ioHandler = ServiceLocator::Instance().Resolve<IOHandler>())
        ioHandler->GetMouse().ExitEditorMode();
    LogManager::Instance().SetLogCallback({});
    for (auto it = m_Panels.rbegin(); it != m_Panels.rend(); ++it) (*it)->Shutdown();
    for (auto it = m_Modules.rbegin(); it != m_Modules.rend(); ++it) (*it)->Shutdown();
    m_Modules.clear();
    m_PanelOwners.clear();
    m_PanelShortcutSlots.clear();
    m_ModuleOwners.clear();
    m_ModuleEnabledBeforeDisable.clear();
    m_Extensions.clear();
    m_Initialized = false;
    m_Panels.clear();
    m_ImGuiLayer.Shutdown();
    ServiceLocator::Instance().Unregister<EditorSelection>();
    m_CommandHistory.Clear();
    ServiceLocator::Instance().Unregister<EditorCommandHistory>();
    ServiceLocator::Instance().Unregister<IKeyboardInputRouter>();
    ServiceLocator::Instance().Unregister<IEditorExtensionRegistry>();
    ServiceLocator::Instance().Unregister<IUIInputCapture>();
    ServiceLocator::Instance().Unregister<EditorViewportState>();
    m_NumberKeyPressed.fill(false);
    m_NudgeTimer = 0.0f;
    m_NudgeUndoCaptured = false;
    m_NudgeEntity = entt::null;
    m_PlaySceneSnapshot.clear();
    m_SavedSceneState.clear();
    m_RunMode = EditorRunMode::Edit;
    m_CheckInitialDockLayout = true;
    m_ResetDockLayoutRequested = false;
}

void EditorSystem::OnUpdate(float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto* ioHandler = sl.Resolve<IOHandler>();
    if (!ioHandler)
        return;

    KeyboardManager& kb = ioHandler->GetKeyboard();
    auto& mouse = ioHandler->GetMouse();
    m_ImGuiLayer.SetPointerInputEnabled(mouse.IsEditorMode());

    auto* scene = sl.Resolve<Scene>();
    if (scene && m_SavedSceneState.empty())
        m_SavedSceneState = CapturePersistedSceneState(*scene);
    auto* geometry = sl.Resolve<IGeometryService>();
    const entt::entity selectedEntity = m_Selection.GetPrimary();
    const bool hasSelection =
        scene && std::any_of(m_Selection.GetAll().begin(), m_Selection.GetAll().end(),
                            [scene](entt::entity entity) { return scene->IsValid(entity); });
    if (hasSelection)
    {
        EnsureSelectionOutlineEffect();
    }
    else if (m_SelectionOutlineHandle != 0)
    {
        if (auto* registry = sl.Resolve<IPostProcessRegistry>())
            registry->UnregisterEffect(m_SelectionOutlineHandle);
        m_SelectionOutlineHandle = 0;
    }
    if (geometry && (mouse.IsEditorMode() || hasSelection))
        geometry->RequestEntityIdBuffer();

    if (hasSelection)
    {
        if (auto* resources = sl.Resolve<ResourceManager>())
        {
            if (auto outline = resources->GetShader(kSelectionOutlineShader))
            {
                std::vector<uint32_t> selectedIds;
                selectedIds.reserve(m_Selection.GetAll().size());
                for (const entt::entity entity : m_Selection.GetAll())
                {
                    if (scene && scene->IsValid(entity))
                        selectedIds.push_back(static_cast<uint32_t>(entt::to_entity(entity)));
                }
                std::sort(selectedIds.begin(), selectedIds.end());
                if (auto* graphics = sl.Resolve<IGraphicsContext>())
                {
                    auto& buffers = graphics->GetBufferManager();
                    const bool newBuffer = m_SelectionBuffer == 0;
                    if (m_SelectionBuffer == 0)
                        m_SelectionBuffer = buffers.GenBuffer();
                    if (newBuffer || selectedIds != m_LastOutlinedEntityIds)
                    {
                        buffers.BindBuffer(BufferType::ShaderStorageBuffer, m_SelectionBuffer);
                        buffers.BufferData(BufferType::ShaderStorageBuffer, selectedIds.size() * sizeof(uint32_t),
                                           selectedIds.data(), BufferUsage::DynamicDraw);
                        m_LastOutlinedEntityIds = selectedIds;
                    }
                    buffers.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::EditorSelectionSSBOBinding,
                                           m_SelectionBuffer);
                }
                outline->use();
                outline->setInt("u_SelectedEntityCount", static_cast<int>(selectedIds.size()));
            }
        }
    }

    const bool pointerInViewport =
        m_ViewportState.rect.Contains(mouse.GetLastX(), mouse.GetLastY()) &&
        !m_ImGuiLayer.WantsPointerInput();
    if (scene && geometry && mouse.IsEditorMouseClicked(Mouse::Left) && pointerInViewport)
    {
        m_BoxSelecting = true;
        m_BoxSelectionAdditive = kb.GetRawKey(Key::LeftControl) || kb.GetRawKey(Key::RightControl);
        m_BoxStartX = m_BoxCurrentX = mouse.GetLastX();
        m_BoxStartY = m_BoxCurrentY = mouse.GetLastY();
    }

    if (m_BoxSelecting)
    {
        m_BoxCurrentX = mouse.GetLastX();
        m_BoxCurrentY = mouse.GetLastY();
    }

    if (scene && geometry && m_BoxSelecting && mouse.IsEditorMouseReleased(Mouse::Left))
    {
        m_BoxSelecting = false;
        const float viewportX = 0.0f;
        const float viewportY = 0.0f;
        const int viewportWidth = ioHandler->GetMonitorManager().GetWidth();
        const int viewportHeight = ioHandler->GetMonitorManager().GetHeight();
        const int bufferWidth = static_cast<int>(geometry->GetPickingBufferWidth());
        const int bufferHeight = static_cast<int>(geometry->GetPickingBufferHeight());
        if (viewportWidth > 0 && viewportHeight > 0 && bufferWidth > 0 && bufferHeight > 0)
        {
            const float dragX = std::abs(m_BoxCurrentX - m_BoxStartX);
            const float dragY = std::abs(m_BoxCurrentY - m_BoxStartY);
            std::vector<entt::entity> selected;
            if (m_BoxSelectionAdditive)
            {
                const auto currentSelection = m_Selection.GetAll();
                selected.assign(currentSelection.begin(), currentSelection.end());
            }

            const auto readEntity = [&](float windowX, float windowY) {
                const float localX = windowX - viewportX;
                const float localY = windowY - viewportY;
                const int x =
                    std::clamp(static_cast<int>(localX * bufferWidth / viewportWidth), 0, bufferWidth - 1);
                const int yFromTop =
                    std::clamp(static_cast<int>(localY * bufferHeight / viewportHeight), 0, bufferHeight - 1);
                uint32_t renderedId = std::numeric_limits<uint32_t>::max();
                if (geometry->ReadEntityId(x, bufferHeight - 1 - yFromTop, renderedId))
                {
                    const entt::entity entity = ResolveRenderedEntity(*scene, renderedId);
                    if (entity != entt::null)
                        selected.push_back(entity);
                }
            };

            if (dragX < 4.0f && dragY < 4.0f)
            {
                readEntity(m_BoxCurrentX, m_BoxCurrentY);
            }
            else
            {
                const int minX = std::clamp(
                    static_cast<int>(std::min(m_BoxStartX, m_BoxCurrentX) - viewportX), 0, viewportWidth - 1);
                const int maxX = std::clamp(
                    static_cast<int>(std::max(m_BoxStartX, m_BoxCurrentX) - viewportX), 0, viewportWidth - 1);
                const int minY = std::clamp(
                    static_cast<int>(std::min(m_BoxStartY, m_BoxCurrentY) - viewportY), 0, viewportHeight - 1);
                const int maxY = std::clamp(
                    static_cast<int>(std::max(m_BoxStartY, m_BoxCurrentY) - viewportY), 0, viewportHeight - 1);
                const int bufferMinX = std::clamp(minX * bufferWidth / viewportWidth, 0, bufferWidth - 1);
                const int bufferMaxX = std::clamp(maxX * bufferWidth / viewportWidth, 0, bufferWidth - 1);
                const int bufferMinY =
                    std::clamp(bufferHeight - 1 - maxY * bufferHeight / viewportHeight, 0, bufferHeight - 1);
                const int bufferMaxY =
                    std::clamp(bufferHeight - 1 - minY * bufferHeight / viewportHeight, 0, bufferHeight - 1);
                std::vector<uint32_t> renderedIds;
                if (geometry->ReadEntityIds(bufferMinX, bufferMinY, bufferMaxX - bufferMinX + 1,
                                            bufferMaxY - bufferMinY + 1, renderedIds))
                {
                    const std::unordered_set<uint32_t> uniqueIds(renderedIds.begin(), renderedIds.end());
                    for (const entt::entity entity : scene->View<InfoComponent>())
                    {
                        if (uniqueIds.contains(static_cast<uint32_t>(entt::to_entity(entity))))
                            selected.push_back(entity);
                    }
                }
            }

            m_Selection.Set(*scene, std::move(selected));
        }
    }
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
        if (panel->IsOpen())
            panel->OnUpdate(dt);
    }

    // Cursor & input management
    auto mode = mouse.GetCursorMode();
    if (mode != CursorMode::Editor)
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
    else
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }

    // MouseManager suppresses game-facing pointer input while Editor owns the
    // cursor; ImGui pointer input is disabled in every other cursor mode.

    bool ctrl = kb.GetRawKey(Key::LeftControl) || kb.GetRawKey(Key::RightControl);
    bool shift = kb.GetRawKey(Key::LeftShift) || kb.GetRawKey(Key::RightShift);
    const EditorModifier modifiers = GetEditorModifiers(kb);
    const bool inputBlocked = m_ImGuiLayer.WantsTextInput();

    if (IsEditorShortcutPressed(kb, Key::S, EditorModifier::Control, m_CtrlSPressed, inputBlocked))
    {
        auto& sm = sl.Require<SceneManager>();
        auto& globalScene = sl.Require<Scene>();
        auto* resourceManager = sl.Resolve<ResourceManager>();
        if (resourceManager)
        {
            bool allSaved = true;
            for (const auto& rec : sm.GetAllScenes())
            {
                if (!rec.filePath.empty())
                {
                    bool saved = false;
                    if (rec.filePath.ends_with(".axsb"))
                    {
                        BinarySceneSerializer serializer;
                        saved = serializer.Serialize(rec.filePath, globalScene);
                    }
                    else
                    {
                        auto* phys = sl.Resolve<IPhysicsWorld>();
                        auto* audio = sl.Resolve<AudioService>();
                        SceneSerializer serializer(*resourceManager, phys, audio);
                        saved = serializer.Serialize(rec.filePath, globalScene, rec.name);
                    }
                    if (saved)
                        LOGGER_INFO("EditorSystem") << "Saved scene: " << rec.name;
                    else
                    {
                        allSaved = false;
                        LOGGER_ERROR("EditorSystem") << "Failed to save scene '" << rec.name
                                                     << "' to " << rec.filePath;
                    }
                }
            }
            if (allSaved)
                m_SavedSceneState = CapturePersistedSceneState(globalScene);
        }
    }

    if (IsEditorShortcutPressed(kb, Key::Z, modifiers, m_ZPressed, inputBlocked))
    {
        auto& globalScene = sl.Require<Scene>();
        if (modifiers == (EditorModifier::Control | EditorModifier::Shift))
            PerformRedo(globalScene);
        else if (modifiers == EditorModifier::Control)
            PerformUndo(globalScene);
    }

    m_NudgeTimer -= dt;
    bool alt = kb.GetRawKey(Key::LeftAlt) || kb.GetRawKey(Key::RightAlt);
    const bool nudgeKeyDown = kb.GetRawKey(Key::Left) || kb.GetRawKey(Key::Right) || kb.GetRawKey(Key::Up) ||
                              kb.GetRawKey(Key::Down) || kb.GetRawKey(Key::PageUp) ||
                              kb.GetRawKey(Key::PageDown);
    if (m_NudgeEntity != selectedEntity)
    {
        m_NudgeEntity = selectedEntity;
        m_NudgeUndoCaptured = false;
    }
    if (!nudgeKeyDown)
        m_NudgeUndoCaptured = false;
    const bool nudgeModifiers = modifiers == EditorModifier::Alt ||
                                modifiers == (EditorModifier::Control | EditorModifier::Alt) ||
                                modifiers == (EditorModifier::Shift | EditorModifier::Alt);
    if (!inputBlocked && nudgeModifiers && selectedEntity != entt::null &&
        m_NudgeTimer <= 0.0f)
    {
        auto& globalScene = sl.Require<Scene>();
        auto& reg = globalScene.GetRegistry();
        if (reg.valid(selectedEntity))
        {
            auto cm = sl.Resolve<ConfigManager>();
            if (cm)
            {
                if (nudgeKeyDown && !m_NudgeUndoCaptured)
                {
                    PushUndoState(globalScene);
                    m_NudgeUndoCaptured = true;
                }
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
                if (ctrl && !shift && reg.all_of<RotationComponent>(selectedEntity))
                {  // Rotate (Ctrl + Alt + Keys)
                    auto& rot = reg.get<RotationComponent>(selectedEntity);
                    if (kb.GetRawKey(Key::Left))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(0, 1, 0));
                        nudged = true;
                    }
                    if (kb.GetRawKey(Key::Right))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(0, 1, 0));
                        nudged = true;
                    }
                    if (kb.GetRawKey(Key::Up))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(1, 0, 0));
                        nudged = true;
                    }
                    if (kb.GetRawKey(Key::Down))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(1, 0, 0));
                        nudged = true;
                    }
                    if (kb.GetRawKey(Key::PageUp))
                    {
                        rot.value = glm::rotate(rot.value, rSnap, glm::vec3(0, 0, 1));
                        nudged = true;
                    }
                    if (kb.GetRawKey(Key::PageDown))
                    {
                        rot.value = glm::rotate(rot.value, -rSnap, glm::vec3(0, 0, 1));
                        nudged = true;
                    }
                    if (nudged)
                        rot.prev = rot.value;
                }
                else if (shift && reg.all_of<ScaleComponent>(selectedEntity))
                {  // Scale (Shift + Alt + Keys)
                    auto& scale = reg.get<ScaleComponent>(selectedEntity);
                    if (snapEnabled)
                    {
                        if (kb.GetRawKey(Key::Left))
                        {
                            scale.value.x = std::round((scale.value.x - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Right))
                        {
                            scale.value.x = std::round((scale.value.x + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Up))
                        {
                            scale.value.z = std::round((scale.value.z - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Down))
                        {
                            scale.value.z = std::round((scale.value.z + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageUp))
                        {
                            scale.value.y = std::round((scale.value.y + sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageDown))
                        {
                            scale.value.y = std::round((scale.value.y - sSnap) / sSnap) * sSnap;
                            nudged = true;
                        }
                    }
                    else
                    {
                        if (kb.GetRawKey(Key::Left))
                        {
                            scale.value.x -= sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Right))
                        {
                            scale.value.x += sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Up))
                        {
                            scale.value.z -= sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Down))
                        {
                            scale.value.z += sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageUp))
                        {
                            scale.value.y += sSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageDown))
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
                else if (!ctrl && !shift && reg.all_of<PositionComponent>(selectedEntity))
                {  // Translate (Alt + Keys)
                    auto& pos = reg.get<PositionComponent>(selectedEntity);
                    if (snapEnabled)
                    {
                        if (kb.GetRawKey(Key::Left))
                        {
                            pos.value.x = std::round((pos.value.x - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Right))
                        {
                            pos.value.x = std::round((pos.value.x + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Up))
                        {
                            pos.value.z = std::round((pos.value.z - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Down))
                        {
                            pos.value.z = std::round((pos.value.z + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageUp))
                        {
                            pos.value.y = std::round((pos.value.y + tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageDown))
                        {
                            pos.value.y = std::round((pos.value.y - tSnap) / tSnap) * tSnap;
                            nudged = true;
                        }
                    }
                    else
                    {
                        if (kb.GetRawKey(Key::Left))
                        {
                            pos.value.x -= tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Right))
                        {
                            pos.value.x += tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Up))
                        {
                            pos.value.z -= tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::Down))
                        {
                            pos.value.z += tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageUp))
                        {
                            pos.value.y += tSnap;
                            nudged = true;
                        }
                        if (kb.GetRawKey(Key::PageDown))
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
                    globalScene.MarkTransformDirty(selectedEntity);
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

        if (IsEditorShortcutPressed(kb, Key::F1, EditorModifier::None, m_F1Pressed, inputBlocked))
        {
            conf.debug.entityNames = !conf.debug.entityNames;
            changed = true;
        }

        if (IsEditorShortcutPressed(kb, Key::F2, EditorModifier::None, m_F2Pressed, inputBlocked))
        {
            conf.debug.gizmos = !conf.debug.gizmos;
            changed = true;
        }

        if (IsEditorShortcutPressed(kb, Key::F3, EditorModifier::None, m_F3Pressed, inputBlocked))
        {
            conf.debug.lightGizmos = !conf.debug.lightGizmos;
            changed = true;
        }

        if (IsEditorShortcutPressed(kb, Key::G, EditorModifier::Control, m_GPressed, inputBlocked))
        {
            conf.debug.gridSnapEnabled = !conf.debug.gridSnapEnabled;
            changed = true;
        }

        if (IsEditorShortcutPressed(kb, Key::H, EditorModifier::Control, m_HPressed, inputBlocked))
        {
            conf.debug.gridIndicatorEnabled = !conf.debug.gridIndicatorEnabled;
            changed = true;
        }

        if (changed)
            cm->UpdateConfig(conf, ConfigChangedEvent::Debug);

        // Reload Active Scene (Ctrl+R)
        if (IsEditorShortcutPressed(kb, Key::R, EditorModifier::Control, m_RPressed, inputBlocked))
        {
            auto& currentScene = ServiceLocator::Instance().Require<Scene>();
            if (IsSceneDirty(currentScene))
                m_RequestReloadConfirmation = true;
            else
                ReloadActiveScene();
        }
        // Toggle Post Process (F6)
        if (IsEditorShortcutPressed(kb, Key::F6, EditorModifier::None, m_F6Pressed, inputBlocked))
        {
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
        // Toggle editor cursor ownership (F10). MouseManager preserves the
        // exact game mode and prevents game code from stealing the cursor.
        if (IsEditorShortcutPressed(kb, Key::F10, EditorModifier::None, m_F10Pressed, inputBlocked))
        {
            mouse.ToggleEditorMode();
            m_ImGuiLayer.SetPointerInputEnabled(mouse.IsEditorMode());
        }
    }

    // Panel toggle shortcuts. Ctrl+1..0 addresses the first bank; Ctrl+Shift+1..0 the second.
    Key numKeys[] = {Key::_1, Key::_2, Key::_3, Key::_4, Key::_5, Key::_6, Key::_7, Key::_8, Key::_9, Key::_0};

    for (int i = 0; i < 10; ++i)
    {
        if (!IsEditorShortcutPressed(kb, numKeys[i], modifiers, m_NumberKeyPressed[i], inputBlocked))
            continue;

        int panelOffset = -1;
        if (modifiers == EditorModifier::Control)
            panelOffset = 0;
        else if (modifiers == (EditorModifier::Control | EditorModifier::Shift))
            panelOffset = 10;

        const int shortcutSlot = panelOffset + i;
        if (panelOffset >= 0)
        {
            const auto slot = std::find(m_PanelShortcutSlots.begin(), m_PanelShortcutSlots.end(), shortcutSlot);
            if (slot != m_PanelShortcutSlots.end())
            {
                const size_t panelIndex = static_cast<size_t>(std::distance(m_PanelShortcutSlots.begin(), slot));
                auto& panel = m_Panels[panelIndex];
                panel->SetOpen(!panel->IsOpen());
            }
        }
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

    if (auto* graphics = ServiceLocator::Instance().Resolve<IGraphicsContext>())
    {
        graphics->GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, 0);
        renderState.SetViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    }

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

    ImGuiID dockspace_id = ImGui::GetID(kDockspaceName);
    if (m_CheckInitialDockLayout)
    {
        // The versioned dockspace id performs a one-time migration back to
        // the overlay editor: the central node is transparent game output.
        if (!ImGui::DockBuilderGetNode(dockspace_id))
            m_ResetDockLayoutRequested = true;
        m_CheckInitialDockLayout = false;
    }
    if (m_ResetDockLayoutRequested)
    {
        BuildDefaultDockLayout(dockspace_id, viewport->WorkSize);
        m_ResetDockLayoutRequested = false;
    }
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    DrawMenuBar();
    const entt::entity viewportCamera = scene.GetActiveCamera();
    m_TransformGizmo.DrawAndUpdate(scene, viewportCamera, 0.0f, 0.0f, width, height);
    if (m_TransformGizmo.IsDragging())
        m_BoxSelecting = false;

    if (m_BoxSelecting)
    {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 minimum(std::min(m_BoxStartX, m_BoxCurrentX), std::min(m_BoxStartY, m_BoxCurrentY));
        const ImVec2 maximum(std::max(m_BoxStartX, m_BoxCurrentX), std::max(m_BoxStartY, m_BoxCurrentY));
        drawList->AddRectFilled(minimum, maximum, IM_COL32(60, 135, 220, 35));
        drawList->AddRect(minimum, maximum, IM_COL32(90, 170, 255, 220), 0.0f, 0, 1.5f);
    }

    for (auto& panel : m_Panels)
    {
        if (panel->IsOpen())
        {
            panel->OnImGui(scene);
        }
    }

    const bool pointerOverPanel = m_ImGuiLayer.WantsPointerInput();
    m_ViewportState.rect = {0.0f, 0.0f, width, height, true, !pointerOverPanel,
                            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)};

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
                    bool allSaved = true;
                    auto scenes = sm.GetAllScenes();
                    for (const auto& sceneRecord : scenes)
                    {
                        if (!sceneRecord.filePath.empty())
                        {
                            bool saved = false;
                            if (sceneRecord.filePath.ends_with(".axsb"))
                            {
                                BinarySceneSerializer serializer;
                                saved = serializer.Serialize(sceneRecord.filePath, globalScene);
                            }
                            else
                            {
                                auto* phys = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
                                auto* audio = ServiceLocator::Instance().Resolve<AudioService>();
                                SceneSerializer serializer(*resourceManager, phys, audio);
                                saved = serializer.Serialize(sceneRecord.filePath, globalScene, sceneRecord.name);
                            }
                            if (saved)
                                LOGGER_INFO("EditorSystem")
                                    << "Saved scene: " << sceneRecord.name << " to " << sceneRecord.filePath;
                            else
                                allSaved = false;
                        }
                    }
                    if (allSaved)
                        m_SavedSceneState = CapturePersistedSceneState(globalScene);
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
            if (ImGui::MenuItem("Reset Layout"))
                m_ResetDockLayoutRequested = true;
            ImGui::Separator();
            for (size_t panelIndex = 0; panelIndex < m_Panels.size(); ++panelIndex)
            {
                auto& panel = m_Panels[panelIndex];
                if (panel->GetGroup() == PanelGroup::Scene)
                {
                    bool open = panel->IsOpen();
                    const int shortcutSlot = m_PanelShortcutSlots[panelIndex];
                    const int keyIndex = shortcutSlot >= 0 ? shortcutSlot % 10 : -1;
                    const char keyName = keyIndex == 9 ? '0' : static_cast<char>('1' + keyIndex);
                    const std::string shortcut = shortcutSlot >= 0 && shortcutSlot < 10
                                                     ? std::string("Ctrl+") + keyName
                                                     : shortcutSlot < 20 && shortcutSlot >= 10
                                                           ? std::string("Ctrl+Shift+") + keyName
                                                           : "";
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
                    const int shortcutSlot = m_PanelShortcutSlots[panelIndex];
                    const int keyIndex = shortcutSlot >= 0 ? shortcutSlot % 10 : -1;
                    const char keyName = keyIndex == 9 ? '0' : static_cast<char>('1' + keyIndex);
                    const std::string shortcut = shortcutSlot >= 0 && shortcutSlot < 10
                                                     ? std::string("Ctrl+") + keyName
                                                     : shortcutSlot < 20 && shortcutSlot >= 10
                                                           ? std::string("Ctrl+Shift+") + keyName
                                                           : "";
                    if (ImGui::MenuItem(panel->GetTitle().c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), &open))
                        panel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            auto& scene = ServiceLocator::Instance().Require<Scene>();
            const bool canUndo = m_CommandHistory.CanUndo();
            const bool canRedo = m_CommandHistory.CanRedo();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
                PerformUndo(scene);
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, canRedo))
                PerformRedo(scene);
            ImGui::Separator();
            ImGui::TextDisabled("%zu undo / %zu redo commands", m_CommandHistory.GetUndoCount(),
                                m_CommandHistory.GetRedoCount());
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

        ImGui::Separator();
        auto& scene = ServiceLocator::Instance().Require<Scene>();
        if (m_RunMode == EditorRunMode::Edit)
        {
            if (ImGui::Button("Play"))
                EnterPlayMode(scene);
        }
        else
        {
            if (ImGui::Button(m_RunMode == EditorRunMode::Paused ? "Resume" : "Pause"))
                TogglePlayPause();
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
                StopPlayMode(scene);
        }

        ImGui::EndMenuBar();
    }

    if (m_RequestReloadConfirmation)
    {
        ImGui::OpenPopup("Reload active scene?");
        m_RequestReloadConfirmation = false;
    }
    if (ImGui::BeginPopupModal("Reload active scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("The active scene contains unsaved editor changes. Reloading will discard them.");
        ImGui::Separator();
        if (ImGui::Button("Discard and Reload"))
        {
            ReloadActiveScene();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static std::string CaptureEditorSceneState(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    if (!rm)
        return {};

    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto* audio = sl.Resolve<AudioService>();
    SceneSerializer serializer(*rm, phys, audio);
    return serializer.SerializeToString(scene, "", true);
}

namespace
{
std::string CapturePersistedSceneState(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    if (!rm)
        return {};
    SceneSerializer serializer(*rm, nullptr, nullptr);
    return serializer.SerializeToString(scene, "", false);
}
}  // namespace

bool EditorSystem::IsSceneDirty(Scene& scene) const
{
    if (m_SavedSceneState.empty())
        return false;
    const std::string current = CapturePersistedSceneState(scene);
    return !current.empty() && current != m_SavedSceneState;
}

bool EditorSystem::ReloadActiveScene()
{
    auto& sl = ServiceLocator::Instance();
    auto* sceneManager = sl.Resolve<SceneManager>();
    auto* scene = sl.Resolve<Scene>();
    if (!sceneManager || !scene)
        return false;

    const std::string activeName = sceneManager->GetActiveScene();
    const auto* record = sceneManager->GetSceneByName(activeName);
    if (!record || record->filePath.empty())
        return false;

    const std::string path = record->filePath;
    sceneManager->UnloadScene(path);
    sceneManager->LoadScene(path);
    sceneManager->SetActiveScene(activeName);
    m_Selection.Clear();
    m_CommandHistory.Clear();
    m_SavedSceneState = CapturePersistedSceneState(*scene);
    LOGGER_INFO("Editor") << "Reloaded active scene: " << activeName;
    return true;
}

static bool RestoreEditorSceneState(Scene& scene, const std::string& state)
{
    if (state.empty())
        return false;

    auto& sl = ServiceLocator::Instance();
    auto* rm = sl.Resolve<ResourceManager>();
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto* audio = sl.Resolve<AudioService>();
    auto* sceneMgr = sl.Resolve<SceneManager>();
    if (!rm)
        return false;

    SceneSerializer validator(*rm, nullptr, nullptr);
    Scene validationScene;
    SceneLoadResult validationResult;
    if (!validator.DeserializeFromString(state, "editor_snapshot_validation", validationScene, validationResult))
    {
        LOGGER_ERROR("EditorSystem") << "Rejected invalid editor scene snapshot; live scene was not modified.";
        return false;
    }

    SceneSerializer serializer(*rm, phys, audio);
    scene.GetRegistry().clear();
    SceneLoadResult restoreResult;
    if (!serializer.DeserializeFromString(state, "editor_snapshot", scene, restoreResult))
    {
        LOGGER_ERROR("EditorSystem") << "Failed to restore validated editor scene snapshot.";
        return false;
    }
    if (sceneMgr)
        sceneMgr->RebuildEntityRecords(scene);

    if (auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>())
        selection->Clear();
    EventManager::Instance().Publish(SceneChangedEvent{&scene.GetRegistry(), &scene});
    return true;
}

bool EditorSystem::EnterPlayMode(Scene& scene)
{
    if (m_RunMode != EditorRunMode::Edit)
        return false;

    m_PlaySceneSnapshot = CaptureEditorSceneState(scene);
    if (m_PlaySceneSnapshot.empty())
    {
        LOGGER_ERROR("EditorSystem") << "Cannot enter Play mode: failed to capture the edit scene.";
        return false;
    }

    if (auto* time = ServiceLocator::Instance().Resolve<TimeService>())
    {
        m_PausedBeforePlay = time->IsPaused();
        m_TimeScaleBeforePlay = time->GetTimeScale();
    }
    if (auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>())
        core->GetEngineLoop().SetPaused(false);
    m_RunMode = EditorRunMode::Play;
    LOGGER_INFO("EditorSystem") << "Entered Play mode; edit scene snapshot captured.";
    return true;
}

bool EditorSystem::StopPlayMode(Scene& scene)
{
    if (m_RunMode == EditorRunMode::Edit || m_PlaySceneSnapshot.empty())
        return false;

    if (!RestoreEditorSceneState(scene, m_PlaySceneSnapshot))
    {
        LOGGER_ERROR("EditorSystem") << "Cannot stop Play mode: edit scene restore failed.";
        return false;
    }

    if (auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>())
    {
        core->GetEngineLoop().SetTimeScale(m_TimeScaleBeforePlay);
        core->GetEngineLoop().SetPaused(m_PausedBeforePlay);
    }
    m_PlaySceneSnapshot.clear();
    m_CommandHistory.Clear();
    BeginTransaction(scene, "Edit scene baseline");
    m_RunMode = EditorRunMode::Edit;
    LOGGER_INFO("EditorSystem") << "Stopped Play mode; edit scene restored.";
    return true;
}

void EditorSystem::TogglePlayPause()
{
    if (m_RunMode == EditorRunMode::Edit)
        return;
    const bool pause = m_RunMode == EditorRunMode::Play;
    if (auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>())
        core->GetEngineLoop().SetPaused(pause);
    m_RunMode = pause ? EditorRunMode::Paused : EditorRunMode::Play;
}

void EditorSystem::PushUndoState(Scene& scene)
{
    BeginTransaction(scene, "Scene edit");
}

void EditorSystem::BeginTransaction(Scene& scene, std::string name)
{
    auto* history = ServiceLocator::Instance().Resolve<EditorCommandHistory>();
    if (!history)
        return;
    history->BeginSceneTransaction(scene, std::move(name), CaptureEditorSceneState, RestoreEditorSceneState);
}

void EditorSystem::BeginPanelTransactionOnContentClick(Scene& scene, std::string name)
{
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;
    const ImGuiContext* context = ImGui::GetCurrentContext();
    if (!context || context->HoveredIdPreviousFrame == 0)
        return;
    const ImVec2 mouse = ImGui::GetMousePos();
    const ImVec2 windowPosition = ImGui::GetWindowPos();
    const ImVec2 contentMinimum = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMaximum = ImGui::GetWindowContentRegionMax();
    const ImVec2 minimum(windowPosition.x + contentMinimum.x, windowPosition.y + contentMinimum.y);
    const ImVec2 maximum(windowPosition.x + contentMaximum.x, windowPosition.y + contentMaximum.y);
    if (mouse.x >= minimum.x && mouse.y >= minimum.y && mouse.x < maximum.x && mouse.y < maximum.y)
        BeginTransaction(scene, std::move(name));
}

bool EditorSystem::ExecuteCommand(Scene& scene, std::unique_ptr<IEditorCommand> command)
{
    auto* history = ServiceLocator::Instance().Resolve<EditorCommandHistory>();
    if (!history)
        return false;
    history->FinalizeSceneTransaction(scene, CaptureEditorSceneState);
    return history->Execute(scene, std::move(command));
}

bool EditorSystem::CommitExecutedCommand(Scene& scene, std::unique_ptr<IEditorCommand> command)
{
    auto* history = ServiceLocator::Instance().Resolve<EditorCommandHistory>();
    return history && history->CommitExecuted(scene, std::move(command), CaptureEditorSceneState);
}

void EditorSystem::PerformUndo(Scene& scene)
{
    if (auto* history = ServiceLocator::Instance().Resolve<EditorCommandHistory>())
        history->Undo(scene, CaptureEditorSceneState);
}

void EditorSystem::PerformRedo(Scene& scene)
{
    if (auto* history = ServiceLocator::Instance().Resolve<EditorCommandHistory>())
        history->Redo(scene);
}

bool EditorSystem::ShouldConsumeKey(Key key) const
{
    if (!m_Enabled)
        return false;

    const auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!io)
        return false;

    // When the editor owns the pointer, keyboard focus belongs to the editor
    // viewport as well. The debug camera still reads the raw keyboard state.
    if (io->GetMouse().IsEditorMode() || m_ImGuiLayer.WantsTextInput())
        return true;

    const EditorModifier modifiers = GetEditorModifiers(io->GetKeyboard());
    const auto exact = [modifiers](EditorModifier required) { return modifiers == required; };

    if (key >= Key::F1 && key <= Key::F9)
        return exact(EditorModifier::None);
    if (key == Key::F10)
        return exact(EditorModifier::None) || exact(EditorModifier::Shift);
    if (key == Key::F11 || key == Key::F12)
        return exact(EditorModifier::None);

    if (key == Key::Delete)
        return exact(EditorModifier::None);

    if ((key == Key::S || key == Key::D || key == Key::R || key == Key::G || key == Key::H) &&
        exact(EditorModifier::Control))
        return true;
    if (key == Key::Z &&
        (exact(EditorModifier::Control) || exact(EditorModifier::Control | EditorModifier::Shift)))
        return true;

    if (key >= Key::_0 && key <= Key::_9 &&
        (exact(EditorModifier::Control) || exact(EditorModifier::Control | EditorModifier::Shift)))
        return true;

    const bool transformKey = key == Key::Left || key == Key::Right || key == Key::Up || key == Key::Down ||
                              key == Key::PageUp || key == Key::PageDown;
    return transformKey &&
           (exact(EditorModifier::Alt) || exact(EditorModifier::Control | EditorModifier::Alt) ||
            exact(EditorModifier::Shift | EditorModifier::Alt));
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
    if (!m_Enabled)
    {
        m_ModuleEnabledBeforeDisable.push_back(module->IsEnabled());
        module->SetEnabled(false);
    }
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
    m_PanelShortcutSlots.push_back(-1);
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
        if (!m_Enabled && index < m_ModuleEnabledBeforeDisable.size())
            m_ModuleEnabledBeforeDisable.erase(m_ModuleEnabledBeforeDisable.begin() +
                                               static_cast<std::ptrdiff_t>(index));
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
        m_PanelShortcutSlots.erase(m_PanelShortcutSlots.begin() + static_cast<std::ptrdiff_t>(index));
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
