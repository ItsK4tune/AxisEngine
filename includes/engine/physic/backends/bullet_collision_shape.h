#pragma once

#include <interface/physics/i_collision_shape.h>
#include <btBulletDynamicsCommon.h>

class BulletCollisionShape : public ICollisionShape
{
public:
    BulletCollisionShape(btCollisionShape* shape, CollisionShapeType type)
        : m_Shape(shape), m_Type(type) {}
    
    ~BulletCollisionShape() { 
        if(m_Shape) delete m_Shape; 
    }

    CollisionShapeType GetType() const override { return m_Type; }

    void SetLocalScaling(const glm::vec3& scaling) override
    {
        if (m_Shape)
            m_Shape->setLocalScaling(btVector3(scaling.x, scaling.y, scaling.z));
    }

    glm::vec3 GetLocalScaling() const override
    {
        if (m_Shape)
        {
            const btVector3& s = m_Shape->getLocalScaling();
            return glm::vec3(s.x(), s.y(), s.z());
        }
        return glm::vec3(1.0f);
    }

    btCollisionShape* GetRaw() const { return m_Shape; }

private:
    btCollisionShape* m_Shape = nullptr;
    CollisionShapeType m_Type;
};
