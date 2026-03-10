#pragma once

#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class SkyboxRenderSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 81; }
    std::string GetName() const override { return "SkyboxRenderSystem"; }
    void Initialize(IGraphicsContext& context);
    void Render(Scene &scene) override;

private:
    EngineContext m_Ctx;
    IGraphicsContext* m_Context = nullptr;
    bool m_Enabled = true;
};