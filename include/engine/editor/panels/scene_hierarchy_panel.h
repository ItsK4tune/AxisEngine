#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>
#include <vector>

struct Scene;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Scene Hierarchy"; }

private:
    void DrawEntityNode(Scene& scene, uint32_t entityId, const std::string& name,
                        const std::string& tag, bool hasChildren,
                        const std::vector<uint32_t>& children);
    uint32_t m_SelectedEntity = UINT32_MAX;
};
#endif
