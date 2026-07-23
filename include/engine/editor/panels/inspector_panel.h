#pragma once

#include <editor/entity_inspector.h>
#include <editor/i_editor_panel.h>

class InspectorPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "Inspector [Ctrl+3]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Scene;
    }

private:
    EntityInspector m_Inspector;
};
