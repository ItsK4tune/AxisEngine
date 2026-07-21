#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class NetworkPanel : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "Network Controller [Ctrl+9]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Tools;
    }

private:
    char m_IpAddress[128] = "127.0.0.1";
    int m_Port = 12345;
    int m_MaxClients = 32;
};
#endif
