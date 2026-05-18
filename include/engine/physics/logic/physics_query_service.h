#pragma once

#include <physics/unit/ray.h>
#include <glm/glm.hpp>

class IPhysicsWorld;
class Scene;

class PhysicsQueryService
{
public:
    PhysicsQueryService();
    ~PhysicsQueryService() = default;

    struct RayHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float distance);
    struct RayHit Raycast(const glm::vec3& start, const glm::vec3& end);
    struct RayHit Raycast(const glm::vec3& origin, float yaw, float pitch, float distance);
    struct RayHit RaycastFromScreen(const glm::vec2& screenPos, float distance = 1000.0f);
};
