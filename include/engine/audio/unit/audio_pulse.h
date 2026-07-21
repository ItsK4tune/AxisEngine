#pragma once

#include <cstddef>
#include <glm/glm.hpp>

namespace AudioPulseLimits
{
inline constexpr size_t MaxPulses = 64;
}

// std430-compatible representation used by custom post-process shaders.
// The two 16-byte rows map to:
//   vec3 origin; float intensity;
//   float peak; float age; float duration; float padding;
struct alignas(16) AudioPulse
{
    glm::vec3 origin{0.0f};
    float intensity = 0.0f;
    float peak = 0.0f;
    float age = 0.0f;
    float duration = 0.0f;
    float padding = 0.0f;
};

static_assert(offsetof(AudioPulse, origin) == 0);
static_assert(offsetof(AudioPulse, intensity) == 12);
static_assert(offsetof(AudioPulse, peak) == 16);
static_assert(offsetof(AudioPulse, age) == 20);
static_assert(offsetof(AudioPulse, duration) == 24);
static_assert(sizeof(AudioPulse) == 32, "AudioPulse must match the std430 shader layout");
