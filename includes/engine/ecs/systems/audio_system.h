#pragma once

#include <ecs/i_system.h>

#include <scene/scene.h>

class SoundPlayer;
#include <systems/audio/sound_player.h>

class AudioSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 30; }
    std::string GetName() const override { return "AudioSystem"; }
    void Update(Scene &scene, float dt) override;
    void StopAll(Scene &scene);

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};
