#pragma once

#include <string>
#include <memory>
#include <functional>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>



struct InfoComponent
{
    std::string name = "Entity";
    std::string tag = "Default";
    std::string sceneName = "";
    uint32_t layer = 1;
    int renderOrder = 0;
    bool isActive = true;
};



struct CameraComponent
{
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
    int screenWidth = 800;
    int screenHeight = 600;

    bool isPrimary = false;
    bool isOrthographic = false;
    float orthoSize = 5.0f;
    uint32_t cullingMask = 0xFFFFFFFF;
};



#define GLM_ENABLE_EXPERIMENTAL

struct PositionComponent {
    glm::vec3 value = glm::vec3(0.0f);
    glm::vec3 prev = glm::vec3(0.0f);
};

struct RotationComponent {
    glm::quat value = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::quat prev = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct ScaleComponent {
    glm::vec3 value = glm::vec3(1.0f);
    glm::vec3 prev = glm::vec3(1.0f);
};

struct HierarchyComponent {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct WorldTransformComponent {
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    glm::mat4 prevWorldMatrix = glm::mat4(1.0f);
    uint32_t version = 0;
    bool isDirty = true;

    glm::mat4 GetInterpolated(float alpha) const
    {
        glm::mat4 result;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                result[r][c] = glm::mix(prevWorldMatrix[r][c], worldMatrix[r][c], alpha);
            }
        }
        return result;
    }
};
