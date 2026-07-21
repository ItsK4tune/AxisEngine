#pragma once

#include <cstdint>

enum class PostProcessInput : uint32_t
{
    None = 0,
    Color = 1u << 0,
    Depth = 1u << 1,
    Normal = 1u << 2,
    // Requests a world-position source. Backends may provide a direct texture,
    // or depth + inverse view-projection with u_WorldPositionFromDepth=true.
    WorldPosition = 1u << 3,
    CameraMatrices = 1u << 4,
    AudioPulses = 1u << 5,
    Standard = 0x3Fu
};

inline PostProcessInput operator|(PostProcessInput left, PostProcessInput right)
{
    return static_cast<PostProcessInput>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
}

inline bool HasPostProcessInput(PostProcessInput value, PostProcessInput input)
{
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(input)) != 0;
}
