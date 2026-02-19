#pragma once

#include <glm/glm.hpp>
#include <memory>

class ICollisionShape;
class IRigidBody;


class IPhysicsWorld
{
public:
    virtual ~IPhysicsWorld() = default;

    virtual void Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void Clear() = 0;

    virtual void SetGravity(const glm::vec3& gravity) = 0;
    virtual void SetMode(int mode) = 0;
    
    virtual void AddRigidBody(IRigidBody* body) = 0;
    virtual void RemoveRigidBody(IRigidBody* body) = 0;


    virtual void DebugDraw() = 0;

    
    virtual std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCompoundShape() = 0;
    virtual void AddChildShape(ICollisionShape* parent, ICollisionShape* child, const glm::vec3& pos, const glm::quat& rot) = 0;
};
