#pragma once

#include <audio/logic/sound_player.h>
#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class SoundPlayer;

class AudioSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    void Update(Scene &scene, float dt) override;
    void StopAll(Scene &scene);

    std::string GetName() const override { return "AudioSystem"; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 70; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};