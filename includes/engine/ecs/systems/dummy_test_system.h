#pragma once

#include <ecs/i_system.h>
#include <core/utils/logger.h>

class DummyTestSystem : public ISystem
{
public:
    void Init(EngineContext ctx) override
    {
        LOGGER_INFO("DummyTestSystem") << "Init() called!";
    }

    void Update(Scene& scene, float dt) override
    {
    }

    bool IsEnabled() const override { return true; }
    void SetEnabled(bool) override {}
    int GetPriority() const override { return 15; }
    std::string GetName() const override { return "DummyTestSystem"; }
};
