#pragma once

#include <core/debug/i_debug_system.h>
#include <core/engine_context.h>

// NullDebugSystem satisfies IDebugSystem with pure no-ops.
// Used in Release builds when ENABLE_DEBUG_SYSTEM is not defined,
// so SystemManager never needs #ifdefs at the call sites.
class NullDebugSystem : public IDebugSystem
{
public:
    void Init(EngineContext) override {}
    void OnUpdate(float) override {}
    void Render(Scene&) override {}
};
