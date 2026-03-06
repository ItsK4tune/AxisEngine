#include <systems/physics/backends/bullet_collision_shape.h>

BulletCollisionShape::~BulletCollisionShape()
{
    if (m_Shape) delete m_Shape;
}

CollisionShapeType BulletCollisionShape::GetType() const
{
    return m_Type;
}

void BulletCollisionShape::SetLocalScaling(const glm::vec3& scaling)
{
    if (m_Shape)
        m_Shape->setLocalScaling(btVector3(scaling.x, scaling.y, scaling.z));
}

glm::vec3 BulletCollisionShape::GetLocalScaling() const
{
    if (m_Shape)
    {
        const btVector3& s = m_Shape->getLocalScaling();
        return glm::vec3(s.x(), s.y(), s.z());
    }
    return glm::vec3(1.0f);
}

BulletMeshCollisionShape::BulletMeshCollisionShape(btCollisionShape* shape, btTriangleIndexVertexArray* indexVertexArray, std::vector<float>&& vertices, std::vector<uint32_t>&& indices)
    : BulletCollisionShape(shape, CollisionShapeType::Mesh), 
      m_IndexVertexArray(indexVertexArray),
      m_Vertices(std::move(vertices)),
      m_Indices(std::move(indices))
{
}

BulletMeshCollisionShape::~BulletMeshCollisionShape()
{
    if (m_IndexVertexArray)
    {
        delete m_IndexVertexArray;
        m_IndexVertexArray = nullptr;
    }
}
