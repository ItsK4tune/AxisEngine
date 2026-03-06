#pragma once

#include <ecs/system.h>
#include <ecs/components/transform_component.h>
#include <scene/scene.h>

class TransformSystem : public ISystem
{
public:
    TransformSystem() : ISystem() {}
    virtual ~TransformSystem() = default;

    void Init(EngineContext ctx) override;
    void Update(Scene& scene, float dt) override;
    void FixedUpdate(Scene& scene, float dt) override;
    void Render(Scene& scene) override;
    void Shutdown() override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }

    std::string GetName() const override { return "TransformSystem"; }
    int GetPriority() const override { return 10; } // High priority, logic needs transforms

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    void UpdateWorldTransform(entt::entity entity, entt::registry& registry, const glm::mat4& parentTransform, bool parentDirty);
};
