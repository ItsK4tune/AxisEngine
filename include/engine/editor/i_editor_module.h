#pragma once

struct Scene;
class KeyboardManager;

class IEditorModule
{
public:
    virtual ~IEditorModule() = default;
    virtual void Initialize()
    {
    }
    virtual void Shutdown()
    {
    }
    virtual void OnUpdate(float)
    {
    }
    virtual void Render(Scene&)
    {
    }
    virtual void ProcessInput(KeyboardManager&)
    {
    }
    virtual bool IsEnabled() const
    {
        return m_Enabled;
    }
    virtual void SetEnabled(bool enabled)
    {
        m_Enabled = enabled;
    }

protected:
    bool m_Enabled = true;
};
