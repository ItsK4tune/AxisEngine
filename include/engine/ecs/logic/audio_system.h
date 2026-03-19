#pragma once

#include <audio/logic/audio_service.h>
#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class AudioService;

class AudioSystem : public IUpdateSystem
{
public:

    void Initialize() override;
    void Update(Scene &scene, float dt) override;
    void StopAll(Scene &scene);

    std::string GetName() const override { return "AudioSystem"; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 70; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    float m_GlobalVolume = 1.0f;
};