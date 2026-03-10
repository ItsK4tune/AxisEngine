#pragma once

#include <core/unit/engine_context.h>
#include <platform/logic/input_system.h>
#include <scene/logic/scene.h>
#include <string>

class IDebugModule {
public:
    virtual ~IDebugModule() = default;
    virtual void Initialize(EngineContext ctx) = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
    virtual void ProcessInput(KeyboardManager& keyboard) = 0;
    virtual std::string GetModuleName() const = 0;
    
    virtual bool IsEnabled() const { return m_Enabled; }
    virtual void SetEnabled(bool enabled) { m_Enabled = enabled; }
    virtual int GetRenderOrder() const { return m_RenderOrder; }
    
protected:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    int m_RenderOrder = 0;
};