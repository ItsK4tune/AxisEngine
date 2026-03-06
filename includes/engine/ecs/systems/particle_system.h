#pragma once

#include <ecs/i_system.h>

#include <resource/resource_manager.h>
#include <scene/scene.h>

class IGraphicsContext;

class ParticleSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 82; }
    std::string GetName() const override { return "ParticleSystem"; }
    void Init(IGraphicsContext& context);
    void Update(Scene &scene, float dt) override;
    void Render(Scene &scene) override;

private:
    EngineContext m_Ctx;
    IGraphicsContext* m_Context = nullptr;
    bool m_Enabled = true;
};
