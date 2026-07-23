#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

#include <memory>

class ISound;

class ResourceBrowserPanel : public IEditorPanel
{
public:
    void Initialize() override
    {
    }
    void OnImGui(Scene& scene) override;
    void DrawContents(Scene& scene);
    std::string GetTitle() const override
    {
        return "Loaded Resources";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Scene;
    }

private:
    int m_ActiveTab = 0;
    std::string m_SelectedName;
    std::string m_SelectedType;
    std::string m_SelectedPath;
    float m_RotX = -0.5f;
    float m_RotY = 0.5f;
    float m_ZoomFactor = 1.0f;
    std::shared_ptr<ISound> m_ActiveSound;
    std::string m_CachedShaderCode;
    std::string m_CachedShaderPath;
};
#endif
