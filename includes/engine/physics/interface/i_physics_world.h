#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <physics/unit/ray_hit.h>

class ICharacterController;
class ICollisionShape;
class IConstraint;
class IRigidBody;

#define GLM_ENABLE_EXPERIMENTAL


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

    virtual void AddCharacterController(ICharacterController* controller) = 0;
    virtual void RemoveCharacterController(ICharacterController* controller) = 0;

    virtual void AddConstraint(std::shared_ptr<IConstraint> constraint) = 0;
    virtual void RemoveConstraint(std::shared_ptr<IConstraint> constraint) = 0;

    virtual void DebugDraw() = 0;

    virtual RayHit Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist) = 0;
    virtual void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) = 0;

    using CollisionFilterCallback = std::function<bool(entt::entity, entt::entity)>;
    virtual void SetCollisionFilter(CollisionFilterCallback callback) = 0;

    virtual std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) = 0;
    virtual std::shared_ptr<ICharacterController> CreateCharacterController(std::shared_ptr<ICollisionShape> shape, float stepHeight = 0.35f) = 0;
    virtual std::shared_ptr<ICollisionShape> CreateCompoundShape() = 0;
    virtual std::shared_ptr<ICollisionShape> CreateMeshShape(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) = 0;
    virtual void AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child, const glm::vec3& pos, const glm::quat& rot) = 0;

    virtual std::shared_ptr<IConstraint> CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB) = 0;
    virtual std::shared_ptr<IConstraint> CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::vec3& axisInA, const glm::vec3& axisInB) = 0;
};