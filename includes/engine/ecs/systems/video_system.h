#pragma once

#include <ecs/i_system.h>

#include <resource/resource_manager.h>
#include <scene/scene.h>

class VideoSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 40; }
    std::string GetName() const override { return "VideoSystem"; }
    void Update(Scene &scene, float dt) override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};
