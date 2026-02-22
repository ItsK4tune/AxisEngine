#pragma once

#include <glm/glm.hpp>
#include <memory>

#include <functional>
#include <entt/entt.hpp>

class ICollisionShape;
class IRigidBody;
class IConstraint;

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

    virtual void AddConstraint(std::shared_ptr<IConstraint> constraint) = 0;
    virtual void RemoveConstraint(std::shared_ptr<IConstraint> constraint) = 0;

    virtual void DebugDraw() = 0;

    using CollisionFilterCallback = std::function<bool(entt::entity, entt::entity)>;
    virtual void SetCollisionFilter(CollisionFilterCallback callback) = 0;

    virtual std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCompoundShape() = 0;
    virtual std::shared_ptr<ICollisionShape> CreateMeshShape(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) = 0;
    virtual void AddChildShape(ICollisionShape* parent, ICollisionShape* child, const glm::vec3& pos, const glm::quat& rot) = 0;

    virtual std::shared_ptr<IConstraint> CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB) = 0;
    virtual std::shared_ptr<IConstraint> CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::vec3& axisInA, const glm::vec3& axisInB) = 0;
};
