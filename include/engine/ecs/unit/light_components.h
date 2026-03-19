#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

// --- Directional Light ---

struct DirectionalLightComponent
{
    glm::vec3 direction = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float ambient = 0.1f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    bool active = true;
    bool isCastShadow = true;
};

// --- Point Light ---

struct PointLightComponent
{
    glm::vec3 color = glm::vec3(1.0f);
    float radius = 10.0f;
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float ambient = 0.1f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    bool active = true;
    bool isCastShadow = false;
};

// --- Spot Light ---

struct SpotLightComponent
{
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float ambient = 0.1f;
    float diffuse = 0.8f;
    float specular = 0.5f;
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));
    bool active = true;
    bool isCastShadow = false;
};
