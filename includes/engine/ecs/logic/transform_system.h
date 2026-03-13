#pragma once

#include <ecs/interface/i_system.h>


#include <ecs/unit/core_components.h>
#include <scene/logic/scene.h>

class TransformSystem : public ISystem
{
public:
    TransformSystem() : ISystem() {}
    virtual ~TransformSystem() = default;

    void Initialize(EngineContext ctx) override;
    void Update(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;
    void FixedUpdate(Scene& scene, float dt) override;
    void Render(Scene& scene) override;
    void Shutdown() override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }

    std::string GetName() const override { return "TransformSystem"; }
    int GetPriority() const override { return 10; }

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    void UpdateWorldTransform(entt::entity entity, entt::registry& registry, const glm::mat4& parentTransform, bool parentDirty, int depth);
};
