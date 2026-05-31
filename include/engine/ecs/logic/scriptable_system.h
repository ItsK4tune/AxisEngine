#pragma once

#include <core/logic/yaml_parser.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <scene/type/scene_events.h>
#include <entt/entt.hpp>
#include <set>
#include <string>
#include <vector>

class ScriptableSystem : public IUpdateSystem, public IECSSystem
{
public:
    void Initialize() override;
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 20;
    }
    std::string GetName() const override
    {
        return "ScriptableSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Script | SystemCategory::Update;
    }
    void Update(Scene& scene, float dt) override;

    void OnSceneChanged(const SceneChangedEvent& e);
    void OnScriptComponentConstructed(entt::registry& reg, entt::entity entity);
    void OnScriptComponentDestroyed(entt::registry& reg, entt::entity entity);

    void OnEntityCollision(const EntityCollisionEvent& e);
    void OnEntityTrigger(const EntityTriggerEvent& e);
    void OnKeyPressed(const KeyPressedEvent& e);
    void OnKeyReleased(const KeyReleasedEvent& e);
    void OnMouseButtonPressed(const MouseButtonPressedEvent& e);
    void OnMouseButtonReleased(const MouseButtonReleasedEvent& e);

    static void LoadScript(Scene& scene, entt::entity entity, const YAMLNode& node);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    uint32_t m_SceneSubId = 0;
    std::vector<uint32_t> m_EventSubs;
    std::set<entt::registry*> m_BoundRegistries;
    Scene* m_ActiveScene = nullptr;
    class TimeService* m_TimeService = nullptr;
    std::vector<entt::entity> m_ScriptEntities;
};

