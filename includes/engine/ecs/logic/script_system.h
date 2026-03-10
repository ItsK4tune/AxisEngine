#pragma once

#include <core/unit/engine_context.h>
#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class ScriptableSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 20; }
    std::string GetName() const override { return "ScriptableSystem"; }
    void Update(Scene &scene, float dt) override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};