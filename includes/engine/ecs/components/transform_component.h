#pragma once

#include <entt/entt.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

struct TransformComponent
{
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
    uint32_t GetVersion() const { return m_Cache.version; }

    void SetDirty(entt::registry &registry);

private:
    struct TransformCache {
        mutable glm::mat4 localMatrix = glm::mat4(1.0f);
        mutable glm::mat4 worldMatrix = glm::mat4(1.0f);

        mutable glm::vec3 lastPosition = glm::vec3(0.0f);
        mutable glm::quat lastRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        mutable glm::vec3 lastScale = glm::vec3(1.0f);

        mutable uint32_t version = 0;
        mutable uint32_t lastParentVersion = 0;
        mutable entt::entity lastParent = entt::null;
        mutable uint32_t lastLocalVersion = 0;

        mutable bool isWorldDirty = true;
    } m_Cache;
};
