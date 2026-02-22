#pragma once

#include <glm/glm.hpp>

struct DirectionalLightComponent
{
    glm::vec3 direction = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(0.4f);
    glm::vec3 specular = glm::vec3(0.5f);
    float intensity = 1.0f;
    bool active = true;
    bool isCastShadow = true;
    bool castShadow = true;
};

struct PointLightComponent
{
    glm::vec3 color = glm::vec3(1.0f);
    float radius = 10.0f;
    glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool active = true;
    bool isCastShadow = false;
};

struct SpotLightComponent
{
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));
    bool active = true;
    bool isCastShadow = false;
};
