#pragma once

#include <core/interface/i_base_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <editor/i_editor_extension_registry.h>
#include <memory>
#include <array>
#include <string>
#include <vector>

struct Scene;

#ifdef ENABLE_EDITOR

#include <editor/i_editor_panel.h>
#include <editor/imgui_layer.h>
#include <platform/interface/cursor_mode.h>
#include <platform/interface/input_codes.h>
#include <resource/unit/font.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>

class IEditorModule;

class EditorSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem, public IEditorExtensionRegistry
{
public:
    EditorSystem();
    ~EditorSystem() override;

    void Initialize() override;
    void Shutdown() override;

    void Update(Scene& scene, float dt) override
    {
        OnUpdate(dt);
    }
    void OnUpdate(float dt);

    SystemCategory GetCategory() const override
    {
        return SystemCategory::PostProcess | SystemCategory::Update | SystemCategory::EditorOverlay;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

    void Render(Scene& scene) override;
    void RenderUIPass(Scene& scene, float width, float height, IRenderStateManager& renderState) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) override;
    std::string GetName() const override
    {
        return "EditorSystem";
    }
    int GetPriority() const override
    {
        return 100;
    }

    std::vector<entt::id_type> GetReadComponents() const override
    {
        return {};
    }
    std::vector<entt::id_type> GetWriteComponents() const override
    {
        return {};
    }

    static void PushUndoState(Scene& scene);
    static void PerformUndo(Scene& scene);
    static void PerformRedo(Scene& scene);

    ImGuiLayer& GetImGuiLayer()
    {
        return m_ImGuiLayer;
    }

    bool RegisterModule(std::string owner, std::string name, ModuleFactory factory) override;
    bool RegisterPanel(std::string owner, std::string name, PanelFactory factory) override;
    size_t UnregisterOwner(std::string_view owner) override;
    std::vector<EditorExtensionInfo> GetExtensions() const override;

private:
    bool m_Enabled = true;

    ImGuiLayer m_ImGuiLayer;
    std::vector<std::unique_ptr<IEditorPanel>> m_Panels;
    std::vector<std::unique_ptr<IEditorModule>> m_Modules;
    std::vector<std::string> m_PanelOwners;
    std::vector<std::string> m_ModuleOwners;
    std::vector<EditorExtensionInfo> m_Extensions;
    bool m_Initialized = false;

    bool m_CtrlSPressed = false;
    bool m_ZPressed = false;
    bool m_YPressed = false;
    bool m_F1Pressed = false;
    bool m_F2Pressed = false;
    bool m_F3Pressed = false;
    bool m_F4Pressed = false;
    bool m_F5Pressed = false;
    bool m_F6Pressed = false;
    bool m_GPressed = false;
    bool m_HPressed = false;
    bool m_RPressed = false;
    float m_NudgeTimer = 0.0f;
    uint64_t m_SelectionOutlineHandle = 0;
    std::array<bool, 10> m_NumberKeyPressed{};
    bool m_ShowAboutPopup = false;

    void DrawMenuBar();
};

#endif
