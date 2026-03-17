#pragma once

#include <core/interface/i_debug_module.h>
#include <core/unit/engine_context.h>
#include <core/logic/debug_core.h>
#include <entt/entity/entity.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <platform/interface/input_codes.h>
#include <render/logic/font.h>
#include <render/logic/shader.h>
#include <render/logic/ui_model.h>
#include <string>
#include <unordered_map>
#include <vector>

class Application;

#ifdef ENABLE_DEBUG_SYSTEM

#define GLM_ENABLE_EXPERIMENTAL


class GizmoDebugModule : public IDebugModule
{
public:
    GizmoDebugModule();
    ~GizmoDebugModule() override;

    void Initialize(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "GizmoDebugModule"; }
    int GetRenderOrder() const override { return 20; }

    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

    bool IsEntityNamesEnabled() const;
    bool IsTransformGizmosEnabled() const;
    bool IsLightGizmosEnabled() const;

private:
    void ToggleEntityNames();
    void ToggleTransformGizmos();
    void ToggleLightGizmos();

    void UpdateDebugLabels(Scene &scene);
    void ClearDebugLabels(Scene &scene);
    void UpdateLightLabels(Scene &scene);
    void ClearLightLabels(Scene &scene);

    void ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action);

    EngineContext m_Ctx;
    bool m_Enabled = true;

    bool m_F3Pressed = false;
    bool m_F4Pressed = false;
    bool m_F5Pressed = false;


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