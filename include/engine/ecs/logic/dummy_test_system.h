#pragma once

#include <core/logic/logger.h>
#include <ecs/interface/i_system.h>

class DummyTestSystem : public IUpdateSystem
{
public:
    void Initialize() override
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