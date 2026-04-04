#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

enum class ReflectionProbeType {
    Static = 0,
    Dynamic = 1
};

struct ReflectionProbeComponent {
    ReflectionProbeType type = ReflectionProbeType::Static;
    float radius = 10.0f;
    int resolution = 512;
    uint32_t cubemapID = 0;
    bool isDirty = true;
    
    // Parallax correction
    bool boxProjection = true;
    glm::vec3 boxMin = glm::vec3(-5.0f);
    glm::vec3 boxMax = glm::vec3(5.0f);
    float blendDistance = 1.0f; // Soft-edge blending distance in meters

    uint32_t currentFace = 0;
};

/**
 * @brief ReflectiveComponent defines how an object reflects the environment.
 */
struct ReflectiveComponent {
    float reflectivity = 1.0f;
    float fresnelPower = 5.0f;
    float fresnelBias = 0.04f;
    bool enabled = true;
};

struct PlanarReflectionComponent {
    uint32_t reflectionTextureID = 0;
    uint32_t reflectionFBO = 0;
    int resolution = 1024;
    bool isDirty = true;
    bool isRendered = false; // Flag to sync rendering
};
