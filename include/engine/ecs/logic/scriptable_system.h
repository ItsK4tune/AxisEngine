#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <string>
#include <vector>
#include <set>
#include <entt/entt.hpp>
#include <core/type/event_types.h>
#include <scene/type/scene_events.h>
#include <core/logic/yaml_parser.h>

class ScriptableSystem : public IUpdateSystem, public IECSSystem
{
public:

    void Initialize() override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 20; }
    std::string GetName() const override { return "ScriptableSystem"; }
    void Update(Scene &scene, float dt) override;

    void OnSceneChanged(const SceneChangedEvent& e);
    void OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity);

    static void LoadScript(Scene &scene, entt::entity entity, const YAMLNode &node);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    uint32_t m_SceneSubId = 0;
    std::set<entt::registry*> m_BoundRegistries;
};