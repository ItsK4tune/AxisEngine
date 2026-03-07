#pragma once

#include <entt/entt.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

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
};


