#pragma once

#include <core/debug/i_debug_system.h>
#include <core/engine_context.h>

class NullDebugSystem : public IDebugSystem
{
public:
    void Init(EngineContext) override {}
    void OnUpdate(float) override {}
    void Render(Scene&) override {}
};
