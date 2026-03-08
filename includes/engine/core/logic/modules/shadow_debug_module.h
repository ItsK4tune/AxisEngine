#pragma once

#include <core/interface/i_debug_module.h>
#include <core/unit/engine_context.h>
#include <string>

class Application;

#ifdef ENABLE_DEBUG_SYSTEM



class ShadowDebugModule : public IDebugModule
{
public:
    ShadowDebugModule();
    ~ShadowDebugModule() override;

    void Init(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene &scene) override;
    void ProcessInput(KeyboardManager &keyboard) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    std::string GetModuleName() const override { return "ShadowDebugModule"; }

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};

#endif