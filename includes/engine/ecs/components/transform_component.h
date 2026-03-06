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

// Legacy class for easier migration if needed, but we should move away from it.
struct TransformComponent
{
    // These will eventually be removed in favor of the SoA components above.
    // For now, it might be used as a proxy or in systems not yet updated.
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 prevPosition = position;
    glm::quat prevRotation = rotation;
    glm::vec3 prevScale = scale;

    entt::entity parent = entt::null;
    std::vector<entt::entity> children;

    glm::mat4 GetLocalModelMatrix() const;
    glm::mat4 GetWorldModelMatrix(entt::registry &registry) const;

    glm::mat4 GetInterpolatedLocalMatrix(float alpha) const;
    glm::mat4 GetInterpolatedWorldMatrix(entt::registry &registry, float alpha) const;

    void SetParent(entt::entity thisEntity, entt::entity newParent, entt::registry &registry, bool keepWorldTransform = false);
    void AddChild(entt::entity thisEntity, entt::entity child, entt::registry &registry, bool keepWorldTransform = false);
    void RemoveChild(entt::entity child);
    bool HasParent() const { return parent != entt::null; }
    
    void SetDirty(entt::registry &registry);
};
