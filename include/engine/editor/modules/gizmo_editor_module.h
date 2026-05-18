#pragma once

#include <editor/i_editor_module.h>
#include <platform/interface/input_codes.h>
#include <resource/unit/font.h>
#include <resource/unit/shader.h>
#include <resource/unit/ui_model.h>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Application;

#ifdef ENABLE_EDITOR

#define GLM_ENABLE_EXPERIMENTAL

class GizmoEditorModule : public IEditorModule
{
public:
    GizmoEditorModule();
    ~GizmoEditorModule() override;

    virtual void Initialize() override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;
    void ProcessInput(KeyboardManager& keyboard) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }
    std::string GetModuleName() const override
    {
        return "GizmoEditorModule";
    }
    int GetRenderOrder() const override
    {
        return 20;
    }

    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

    bool IsEntityNamesEnabled() const;
    bool IsTransformGizmosEnabled() const;
    bool IsLightGizmosEnabled() const;

private:
    void ToggleEntityNames();
    void ToggleTransformGizmos();
    void ToggleLightGizmos();

    void UpdateDebugLabels(Scene& scene);
    void ClearDebugLabels(Scene& scene);
    void UpdateLightLabels(Scene& scene);
    void ClearLightLabels(Scene& scene);

    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

    bool m_Enabled = true;

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
