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
#include <unordered_map>
#include <vector>

class Application;

#ifdef ENABLE_EDITOR


class GizmoEditorModule : public IEditorModule
{
public:
    GizmoEditorModule() = default;
    ~GizmoEditorModule() override;

    void Initialize() override;
    void Shutdown() override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;
    void SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad);

private:
    bool IsEntityNamesEnabled() const;
    bool IsTransformGizmosEnabled() const;
    bool IsLightGizmosEnabled() const;

    void UpdateDebugLabels(Scene& scene);
    void ClearDebugLabels(Scene& scene);
    void UpdateLightLabels(Scene& scene);
    void ClearLightLabels(Scene& scene);
    void ClearSceneLabels();

    void ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState, std::function<void()> action);

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
    int m_SceneUnloadedSubId = -1;
    std::vector<DebugLineVertex> m_GizmoLines;
};

#endif
