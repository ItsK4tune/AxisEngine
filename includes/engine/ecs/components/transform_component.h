#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entt.hpp>
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
    uint32_t GetVersion() const { return m_Version; }

    void SetDirty(entt::registry &registry);

private:
    mutable glm::mat4 m_LocalMatrix = glm::mat4(1.0f);
    mutable glm::mat4 m_WorldMatrix = glm::mat4(1.0f);

    mutable glm::vec3 m_LastPosition = glm::vec3(0.0f);
    mutable glm::quat m_LastRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    mutable glm::vec3 m_LastScale = glm::vec3(1.0f);

    mutable uint32_t m_Version = 0;
    mutable uint32_t m_LastParentVersion = 0;
    mutable entt::entity m_LastParent = entt::null;
    mutable uint32_t m_LastLocalVersion = 0;

    mutable bool m_IsWorldDirty = true;
};
