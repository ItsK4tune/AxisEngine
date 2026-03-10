#pragma once

#include <audio/logic/sound_player.h>
#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class SoundPlayer;

class AudioSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
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