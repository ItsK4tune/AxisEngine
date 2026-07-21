#pragma once

#include <glm/glm.hpp>
#include <vector>

struct LightProbeComponent
{
    // 9 Spherical Harmonics coefficients for RGB
    // Representing L00, L1-1, L10, L11, L2-2, L2-1, L20, L21, L22
    glm::vec3 sh[9] = {glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f),
                       glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};

    float intensity = 1.0f;
    float radius = 5.0f;
    glm::vec3 tint = glm::vec3(1.0f);
};
