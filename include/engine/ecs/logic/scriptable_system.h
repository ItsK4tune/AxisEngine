#pragma once

#include <ecs/interface/i_system.h>
#include <string>
#include <vector>
#include <entt/entt.hpp>

class ScriptableSystem : public IUpdateSystem
{
public:

    void Initialize() override {}
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 20; }
    std::string GetName() const override { return "ScriptableSystem"; }
    void Update(Scene &scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
};