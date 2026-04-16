#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class ToolsPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Tools"; }

    bool GetWireframe()   const { return m_Wireframe; }
    bool GetNoTexture()   const { return m_NoTexture; }
    bool GetShadows()     const { return m_Shadows; }
    bool GetSkybox()      const { return m_Skybox; }
    bool GetPhysicsDebug()const { return m_PhysicsDebug; }
    bool GetUIEnabled()   const { return m_UIEnabled; }
    bool GetEntityNames() const { return m_EntityNames; }
    bool GetGizmos()      const { return m_Gizmos; }
    bool GetLightGizmos() const { return m_LightGizmos; }

private:
    bool m_Wireframe   = false;
    bool m_NoTexture   = false;
    bool m_Shadows     = true;
    bool m_Skybox      = true;
    bool m_PhysicsDebug= false;
    bool m_UIEnabled   = true;
    bool m_EntityNames = false;
    bool m_Gizmos      = false;
    bool m_LightGizmos = false;
};
#endif
