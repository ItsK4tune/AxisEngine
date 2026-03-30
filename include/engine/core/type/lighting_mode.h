#pragma once

#include <cstdint>

enum class LightingMode : uint32_t
{
    Bake = 0,
    LightProbe = 1,
    ReflectionProbes = 2,
    RealTime = 3,
};
