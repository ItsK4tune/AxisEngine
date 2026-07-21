#pragma once

#include <cstdint>

enum class SpatialCullingMode : uint8_t
{
    Auto = 0,
    Linear,
    Octree
};

inline constexpr const char* SpatialCullingModeName(SpatialCullingMode mode)
{
    switch (mode)
    {
        case SpatialCullingMode::Auto:
            return "Auto";
        case SpatialCullingMode::Linear:
            return "Linear";
        case SpatialCullingMode::Octree:
            return "Octree";
    }
    return "Auto";
}
