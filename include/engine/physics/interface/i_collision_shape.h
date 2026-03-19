#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL

enum class CollisionShapeType
{
    Box,
    Sphere,
    Capsule,
    Mesh,
    Heightfield,
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

    void AddChild(std::shared_ptr<ICollisionShape> child) {
        m_Children.push_back(child);
    }

protected:
    std::vector<std::shared_ptr<ICollisionShape>> m_Children;
};