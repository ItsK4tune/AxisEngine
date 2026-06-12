#pragma once
#include <physics/interface/i_collision_shape.h>
#include <geometry/PxGeometryHelpers.h>
#include <memory>

class PhysXCollisionShape : public ICollisionShape
{
public:
    PhysXCollisionShape(const physx::PxGeometry& geometry, CollisionShapeType type)
        : m_Type(type), m_GeometryHolder(geometry)
    {
    }

    ~PhysXCollisionShape() override = default;

    CollisionShapeType GetType() const override { return m_Type; }
    void SetLocalScaling(const glm::vec3& scaling) override { m_Scaling = scaling; }
    glm::vec3 GetLocalScaling() const override { return m_Scaling; }

    const physx::PxGeometry& GetGeometry() const { return m_GeometryHolder.any(); }

private:
    CollisionShapeType m_Type;
    physx::PxGeometryHolder m_GeometryHolder;
    glm::vec3 m_Scaling{1.0f};
};
