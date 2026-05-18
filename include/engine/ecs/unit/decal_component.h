#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct DecalComponent
{
    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 invModel = glm::mat4(1.0f);

    float opacity = 1.0f;
    float roughness = 1.0f;
    float metallic = 0.0f;
    float reflectivity = 0.0f;
    glm::vec4 tintColor = glm::vec4(1.0f);

    float lifetime = -1.0f;
    uint32_t renderOrder = 0;
    int lightingMode = 0;  // 0=Unlit (None), 1=Lit, 2=Lit+Shadow

    std::vector<std::string> targetTags;
    std::string customShader;
};
