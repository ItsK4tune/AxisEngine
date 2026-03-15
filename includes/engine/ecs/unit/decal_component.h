#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <entt/entt.hpp>

struct DecalComponent
{
    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;
    
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 invModel = glm::mat4(1.0f);
    
    float opacity = 1.0f;
    float lifetime = -1.0f; // -1 means infinite
    uint32_t renderOrder = 0;
    
    // Tag filtering: only project onto entities with these tags
    // If empty, project onto everything (default)
    std::vector<std::string> targetTags;
};
