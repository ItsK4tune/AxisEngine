#pragma once
#include <physics/interface/i_physics_world.h>
#include <physics/strategy/physx/physx_collision_shape.h>
#include <physics/strategy/physx/physx_rigid_body.h>
#include <physics/strategy/physx/physx_character_controller.h>
#include <physics/strategy/physx/physx_constraint.h>
#include <PxPhysicsAPI.h>
#include <memory>
#include <vector>

class PhysXPhysicsWorld : public IPhysicsWorld
{
public:
    PhysXPhysicsWorld();
    ~PhysXPhysicsWorld();

    void Initialize() override;
    void Update(float dt) override;
    void Clear() override;

    void SetGravity(const glm::vec3& gravity) override;
    void SetMode(int mode) override;
    void SetSimulationSettings(float fixedTimeStep, int maxSubSteps) override;
    void SetSolverIterations(int iterations) override;
    void SetCCDEnabled(bool enabled, float threshold = 0.0f) override;

    void AddRigidBody(IRigidBody* body) override;
    void RemoveRigidBody(IRigidBody* body) override;

    void AddCharacterController(ICharacterController* controller) override;
    void RemoveCharacterController(ICharacterController* controller) override;

    void AddConstraint(std::shared_ptr<IConstraint> constraint) override;
    void RemoveConstraint(std::shared_ptr<IConstraint> constraint) override;

    void DebugDraw() override;

    RayHit Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                   entt::entity ignore = entt::null) override;
    void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override;
    void SyncRigidBody(IRigidBody* body, const glm::vec3& pos, const glm::quat& rot) override;

    void SetCollisionFilter(CollisionFilterCallback callback) override;

    std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot,
                                                std::shared_ptr<ICollisionShape> shape) override;
    std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) override;
    std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) override;
    std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) override;
    std::shared_ptr<ICollisionShape> CreateCylinderShape(float radius, float height) override;
    std::shared_ptr<ICharacterController> CreateCharacterController(std::shared_ptr<ICollisionShape> shape,
                                                                    float stepHeight = 0.35f) override;
    std::shared_ptr<ICollisionShape> CreateCompoundShape() override;
    std::shared_ptr<ICollisionShape> CreateMeshShape(const std::vector<float>& vertices,
                                                     const std::vector<uint32_t>& indices) override;
    std::shared_ptr<ICollisionShape> CreateHeightfieldShape(const std::vector<float>& heights, int width, int length,
                                                            float minHeight, float maxHeight) override;
    void AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child,
                       const glm::vec3& pos, const glm::quat& rot) override;

    std::shared_ptr<IConstraint> CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA,
                                                             std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA,
                                                             const glm::vec3& pivotInB) override;
    std::shared_ptr<IConstraint> CreateFixedConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB,
                                                       const glm::vec3& pivotInA, const glm::vec3& pivotInB,
                                                       const glm::quat& rotInA, const glm::quat& rotInB) override;
    std::shared_ptr<IConstraint> CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB,
                                                       const glm::vec3& pivotInA, const glm::vec3& pivotInB,
                                                       const glm::vec3& axisInA, const glm::vec3& axisInB) override;

    std::vector<CollisionInfo> GetActiveCollisions() override;

    physx::PxPhysics* GetPhysics() const { return m_Physics; }
    physx::PxScene* GetScene() const { return m_Scene; }
    physx::PxMaterial* GetDefaultMaterial() const { return m_DefaultMaterial; }

private:
    physx::PxDefaultAllocator m_Allocator;
    physx::PxDefaultErrorCallback m_ErrorCallback;
    physx::PxFoundation* m_Foundation = nullptr;
    physx::PxPhysics* m_Physics = nullptr;
    physx::PxDefaultCpuDispatcher* m_CpuDispatcher = nullptr;
    physx::PxScene* m_Scene = nullptr;
    physx::PxMaterial* m_DefaultMaterial = nullptr;
    physx::PxControllerManager* m_ControllerManager = nullptr;

    CollisionFilterCallback m_FilterCallback;
    float m_FixedTimeStep = 1.0f / 60.0f;
    float m_Accumulator = 0.0f;
    std::vector<CollisionInfo> m_ActiveCollisions;
    std::vector<PhysXCharacterController*> m_CharacterControllers;
};
