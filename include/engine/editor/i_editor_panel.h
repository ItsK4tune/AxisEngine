#pragma once

#include <string>

struct Scene;

enum class PanelGroup
{
    Scene,
    Debug,
    Tools,
    Help
};

class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;
    virtual void Initialize()
    {
    }
    virtual void Shutdown()
    {
    }
    virtual void OnUpdate(float dt)
    {
    }
    virtual void OnImGui(Scene& scene) = 0;
    virtual std::string GetTitle() const = 0;
    virtual PanelGroup GetGroup() const = 0;
    virtual bool IsOpen() const
    {
        return m_Open;
    }
    virtual void SetOpen(bool v)
    {
        m_Open = v;
    }

protected:
    bool m_Open = false;
};
