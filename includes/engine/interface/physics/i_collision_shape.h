#pragma once

#include <glm/glm.hpp>

enum class CollisionShapeType
{
    Box,
    Sphere,
    Capsule,
    Mesh,
    ConvexHull,
    CompoundHull
};

class ICollisionShape
{
public:
    virtual ~ICollisionShape() = default;

    virtual CollisionShapeType GetType() const = 0;
    virtual void SetLocalScaling(const glm::vec3& scaling) = 0;
    virtual glm::vec3 GetLocalScaling() const = 0;
};
