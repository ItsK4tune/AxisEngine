#pragma once

#include <core/logic/event_manager.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <scene/logic/scene.h>
#include <glm/glm.hpp>
#include <limits>
#include <unordered_map>

class AudioService;

class AudioSystem : public IUpdateSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Update(Scene& scene, float dt) override;
    void StopAll(Scene& scene);

    std::string GetName() const override
    {
        return "AudioSystem";
    }
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override;
    int GetPriority() const override
    {
        return 70;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Audio;
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    struct AppliedSourceState
    {
        class ISound* sound = nullptr;
        float volume = std::numeric_limits<float>::quiet_NaN();
        float pitch = std::numeric_limits<float>::quiet_NaN();
        float pan = std::numeric_limits<float>::quiet_NaN();
        float minDistance = std::numeric_limits<float>::quiet_NaN();
        float maxDistance = std::numeric_limits<float>::quiet_NaN();
        glm::vec3 velocity{std::numeric_limits<float>::quiet_NaN()};
        glm::vec3 position{std::numeric_limits<float>::quiet_NaN()};
        uint32_t transformVersion = 0;
    };

    void OnAudioSourceDestroyed(entt::registry& registry, entt::entity entity);

    bool m_Enabled = true;
    float m_GlobalVolume = 100.0f;
    float m_AppliedGlobalVolume = std::numeric_limits<float>::quiet_NaN();
    std::unordered_map<entt::entity, AppliedSourceState> m_AppliedSources;
    Scene* m_BoundScene = nullptr;
    EventSubscriptionList m_EventSubscriptions;
};
