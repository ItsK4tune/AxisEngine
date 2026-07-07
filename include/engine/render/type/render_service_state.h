#pragma once

#include <ecs/interface/i_render_service.h>
#include <glm/glm.hpp>

struct RenderCameraState
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

struct RenderFlagsState
{
    bool enabled = true;
    bool instanceBatchingEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    bool renderOrderEnabled = true;
    uint32_t filterLayerMask = 0xFFFFFFFF;
    float distanceCullingSq = 0.0f;
    AntiAliasingMode aaMode = AntiAliasingMode::NONE;
};

struct RenderIBLState
{
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
    unsigned int brdfLUT = 0;
};

struct RenderGlobalState
{
    glm::mat4 prevViewProj = glm::mat4(1.0f);
    glm::mat4 currViewProj = glm::mat4(1.0f);
    glm::mat4 jitteredProjection = glm::mat4(1.0f);
    glm::vec2 jitterOffset = glm::vec2(0.0f);
    int frameIndex = 0;
};
