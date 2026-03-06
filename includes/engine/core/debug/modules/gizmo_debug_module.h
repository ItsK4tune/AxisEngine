#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <core/engine_context.h>
#include <entt/entity/entity.hpp>
#include <functional>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <rendering/core/shader.h>
#include <rendering/renderer/font.h>
#include <rendering/renderer/ui_model.h>
#include <core/debug/interfaces/i_debug_module.h>
#include <systems/window/interfaces/input_codes.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Application;

class GizmoDebugModule : public IDebugModule
{
public:
    GizmoDebugModule();
    ~GizmoDebugModule() override;

    void Init(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "GizmoDebugModule"; }
    int GetRenderOrder() const override { return 20; }

    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

    bool IsEntityNamesEnabled() const { return m_ShowEntityNames; }
    bool IsTransformGizmosEnabled() const { return m_ShowTransformGizmos; }
    bool IsLightGizmosEnabled() const { return m_ShowLightGizmos; }

private:
    void ToggleEntityNames() { m_ShowEntityNames = !m_ShowEntityNames; }
    void ToggleTransformGizmos() { m_ShowTransformGizmos = !m_ShowTransformGizmos; }
    void ToggleLightGizmos() { m_ShowLightGizmos = !m_ShowLightGizmos; }

    void UpdateDebugLabels(Scene &scene);
    void ClearDebugLabels(Scene &scene);
    void UpdateLightLabels(Scene &scene);
    void ClearLightLabels(Scene &scene);

    void ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action);

    EngineContext m_Ctx;
    bool m_Enabled = true;

    bool m_F3Pressed = false;
    bool m_F4Pressed = false;
    bool m_F5Pressed = false;

    bool m_ShowEntityNames = false;
    bool m_ShowTransformGizmos = false;
    bool m_ShowLightGizmos = false;

    std::unordered_map<entt::entity, entt::entity> m_EntityLabelMap;
    std::unordered_map<entt::entity, entt::entity> m_LightLabelMap;

    std::shared_ptr<Font> m_DebugFont = nullptr;
    std::shared_ptr<Shader> m_TextShader = nullptr;
    std::shared_ptr<UIModel> m_TextQuad = nullptr;

    struct DebugLineVertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    unsigned int m_LineVAO = 0;
    unsigned int m_LineVBO = 0;
    std::vector<DebugLineVertex> m_GizmoLines;
};

#endif
