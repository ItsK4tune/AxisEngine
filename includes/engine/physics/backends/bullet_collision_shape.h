#pragma once

#include <btBulletDynamicsCommon.h>
#include <physics/interfaces/i_collision_shape.h>

class BulletCollisionShape : public ICollisionShape
{
public:
    BulletCollisionShape(btCollisionShape* shape, CollisionShapeType type)
        : m_Shape(shape), m_Type(type) {}

    ~BulletCollisionShape();

    CollisionShapeType GetType() const override;

    void SetLocalScaling(const glm::vec3& scaling) override;
    glm::vec3 GetLocalScaling() const override;

    btCollisionShape* GetRaw() const { return m_Shape; }

private:
    btCollisionShape* m_Shape = nullptr;
    CollisionShapeType m_Type;
};

class BulletMeshCollisionShape : public BulletCollisionShape
{
public:
    BulletMeshCollisionShape(btCollisionShape* shape, btTriangleIndexVertexArray* indexVertexArray, std::vector<float>&& vertices, std::vector<uint32_t>&& indices);
    ~BulletMeshCollisionShape();

private:
    btTriangleIndexVertexArray* m_IndexVertexArray = nullptr;
    std::vector<float> m_Vertices;
    std::vector<uint32_t> m_Indices;
};
