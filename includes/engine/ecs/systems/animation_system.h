#pragma once

#include <ecs/i_system.h>

#include <scene/scene.h>

class AnimationSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 50; }
    std::string GetName() const override { return "AnimationSystem"; }
    void Update(Scene &scene, float dt) override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    std::vector<entt::entity> m_Entities;
};
