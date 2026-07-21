#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

enum class ReflectionProbeType
{
    Static = 0,
    Dynamic = 1
};

struct ReflectionProbeComponent
{
    ReflectionProbeType type = ReflectionProbeType::Static;
    int resolution = 512;
    uint32_t cubemapID = 0;
    bool isDirty = true;

    // Parallax correction
    bool boxProjection = true;
    glm::vec3 boxMin = glm::vec3(-1000000.0f);
    glm::vec3 boxMax = glm::vec3(1000000.0f);
    float blendDistance = 1.0f;  // Soft-edge blending distance in meters

    uint32_t currentFace = 0;
    int lastGpuIndex = -1;   // New field for Deferred Reflection Binding
    int lastResolution = 0;  // Tracks allocated cubemap resolution for reallocation detection
};

/**
 * @brief ReflectiveComponent defines how an object reflects the environment.
 */
struct ReflectiveComponent
{
    float reflectivity = 1.0f;
    float fresnelPower = 5.0f;
    float fresnelBias = 0.04f;
    bool enabled = true;
    std::string targetProbe = "";
};

struct PlanarReflectionComponent
{
    uint32_t reflectionTextureID = 0;
    uint32_t reflectionFBO = 0;
    uint32_t reflectionDepthRBO = 0;
    int resolution = 1024;
    int resolution_y = 1024;
    float resolutionScale = 0.5f;
    uint32_t updateIntervalFrames = 1;
    bool isDirty = true;
    bool isRendered = false;                // Flag to sync rendering
    glm::vec3 normal = glm::vec3(0, 1, 0);  // World-space normal for shader masking

    // Runtime scheduler state.
    uint32_t framesUntilUpdate = 0;
};
