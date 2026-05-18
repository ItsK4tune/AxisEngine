#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <cstdint>

struct RenderViewParams
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::vec3 cameraPos = glm::vec3(0.0f);
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float lodFactor = 1.0f;
    int width = 800;
    int height = 600;
    uint32_t cullingMask = 0xFFFFFFFF;
    bool isCapturingProbe = false;
    entt::entity excludeEntity = (entt::entity)0xFFFFFFFF;
};
