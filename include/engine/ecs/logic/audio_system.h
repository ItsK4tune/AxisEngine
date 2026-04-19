#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <scene/logic/scene.h>

class AudioService;

class AudioSystem : public IUpdateSystem, public IECSSystem
{
public:

    void Initialize() override;
    void Update(Scene &scene, float dt) override;
    void StopAll(Scene &scene);

    std::string GetName() const override { return "AudioSystem"; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 70; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Audio; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    void OnAudioSourceDestroyed(entt::registry& registry, entt::entity entity);

    bool m_Enabled = true;
    float m_GlobalVolume = 1.0f;
};