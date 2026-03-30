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
};
