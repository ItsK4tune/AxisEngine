#pragma once

#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class AnimationSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 50; }
    std::string GetName() const override { return "AnimationSystem"; }
    void Update(Scene &scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    std::vector<entt::entity> m_Entities;
};