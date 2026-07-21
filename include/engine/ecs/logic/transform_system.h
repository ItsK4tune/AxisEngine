#pragma once

#include <core/logic/event_manager.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/unit/core_components.h>
#include <scene/logic/scene.h>
#include <unordered_set>
#include <unordered_map>

class TransformSystem : public IUpdateSystem, public IECSSystem
{
public:
    TransformSystem() : IBaseSystem()
    {
    }
    virtual ~TransformSystem() = default;

    void Initialize() override;
    void Shutdown() override;
    void Update(Scene& scene, float dt) override;
    void FixedUpdate(Scene& scene, float fixedDt) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }

    std::string GetName() const override
    {
        return "TransformSystem";
    }
    int GetPriority() const override
    {
        return 90;
    }
    bool WantsFixedUpdate() const override
    {
        return true;
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    std::vector<entt::entity> m_LinearTransforms;
    bool m_IsLinearTransformsDirty = true;
    void MarkTransformGraphDirty()
    {
        m_IsLinearTransformsDirty = true;
    }
    void MarkTransformDirty(entt::entity entity)
    {
        m_DirtyTransforms.insert(entity);
    }
    void BindRegistry(Scene& scene);
    void UnbindRegistries();
    void RebuildLinearTransforms(Scene& scene);
    void OnHierarchyChanged(entt::registry& reg, entt::entity entity);
    void OnTransformChanged(entt::registry& reg, entt::entity entity);

    bool m_Enabled = true;

private:
    std::unordered_set<entt::registry*> m_BoundRegistries;
    bool m_HasParentedTransforms = false;
    std::unordered_set<entt::entity> m_DirtyTransforms;
    std::unordered_map<entt::entity, size_t> m_LinearTransformIndices;
    std::vector<entt::entity> m_DirtyScratch;
    EventSubscriptionList m_EventSubscriptions;
};
