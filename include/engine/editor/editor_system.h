#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_base_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <memory>
#include <string>
#include <vector>

struct Scene;

#ifdef ENABLE_EDITOR

// Legacy DebugConfig removed -- Use AppConfig::debug instead

#include <resource/unit/font.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>
#include <editor/i_editor_panel.h>
#include <editor/imgui_layer.h>
#include <platform/interface/cursor_mode.h>
#include <platform/interface/input_codes.h>

class IEditorModule;

class EditorSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    EditorSystem();
    ~EditorSystem();

    void Initialize() override;
    void Shutdown() override;
    
    void Update(Scene& scene, float dt) override { OnUpdate(dt); }
    void OnUpdate(float dt);
    
    SystemCategory GetCategory() const override { return SystemCategory::PostProcess | SystemCategory::Update | SystemCategory::EditorOverlay; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Graphics; }

    void Render(Scene& scene) override;
    void RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState) override;
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetName() const override { return "EditorSystem"; }
    int GetPriority() const override { return 100; }
 
    std::vector<entt::id_type> GetReadComponents() const override { return {}; }
    std::vector<entt::id_type> GetWriteComponents() const override { return {}; }

    static void PushUndoState(Scene& scene);
    static void PerformUndo(Scene& scene);

private:
    bool m_Enabled = true;

    ImGuiLayer m_ImGuiLayer;
    std::vector<std::unique_ptr<IEditorPanel>> m_Panels;

    // We still keep the underlying debug modules alive so the panels can proxy to them
    std::vector<std::unique_ptr<IEditorModule>> m_Modules;

    // Track cursor mode prior to hovering a panel
    CursorMode m_PreHoverCursorMode;
    bool m_WasHoveringPanel = false;
    bool m_CtrlSPressed = false;

    void DrawMenuBar();
};

#else

class NullEditorSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override {}
    void Shutdown() override {}
    void Update(Scene&, float) override {}
    void OnUpdate(float) {}
    void Render(Scene&) override {}
    void RenderUIPass(Scene&, float, float, IRenderStateManager&) override {}
    bool IsEnabled() const override { return false; }
    void SetEnabled(bool) override {}
    std::string GetName() const override { return "EditorSystem"; }
    int GetPriority() const override { return 1000; }
    SystemCategory GetCategory() const override { return SystemCategory::EditorOverlay; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Graphics; }
    
    std::vector<entt::id_type> GetReadComponents() const override { return {}; }
    std::vector<entt::id_type> GetWriteComponents() const override { return {}; }
};

#endif
