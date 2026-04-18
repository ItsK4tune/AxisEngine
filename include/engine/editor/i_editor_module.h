#pragma once

#include <platform/logic/input_manager.h>
#include <scene/logic/scene.h>
#include <string>

class IEditorModule {
public:
    virtual ~IEditorModule() = default;
    virtual void Initialize() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
    virtual void ProcessInput(KeyboardManager& keyboard) = 0;
    virtual std::string GetModuleName() const = 0;
    
    virtual bool IsEnabled() const { return m_Enabled; }
    virtual void SetEnabled(bool enabled) { m_Enabled = enabled; }
    virtual int GetRenderOrder() const { return m_RenderOrder; }
    
protected:
    bool m_Enabled = true;
    int m_RenderOrder = 0;
};