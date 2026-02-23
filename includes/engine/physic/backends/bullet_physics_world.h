#pragma once

#include <interface/physics/i_physics_world.h>
#include <physic/backends/bullet_rigid_body.h>
#include <physic/backends/bullet_collision_shape.h>
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <memory>
#include <physic/backends/bullet_debug_drawer.h>

class CustomCollisionDispatcher : public btCollisionDispatcher
{
public:
    using CollisionFilterCallback = std::function<bool(entt::entity, entt::entity)>;

    CustomCollisionDispatcher(btCollisionConfiguration* collisionConfiguration)
        : btCollisionDispatcher(collisionConfiguration) {}

    bool needsCollision(const btCollisionObject* body0, const btCollisionObject* body1) override
    {
        if (m_FilterCallback)
        {
            entt::entity eA = (entt::entity)(uintptr_t)body0->getUserPointer();
            entt::entity eB = (entt::entity)(uintptr_t)body1->getUserPointer();
            
            if (!m_FilterCallback(eA, eB))
                return false;
        }

        return btCollisionDispatcher::needsCollision(body0, body1);
    }

    void SetFilterCallback(CollisionFilterCallback callback)
    {
        m_FilterCallback = callback;
    }

private:
    CollisionFilterCallback m_FilterCallback;
};

class BulletPhysicsWorld : public IPhysicsWorld
{
public:
    BulletPhysicsWorld();
    ~BulletPhysicsWorld();

    void Init() override;
    void Update(float dt) override;
    void Clear() override;

    void SetGravity(const glm::vec3& gravity) override;
    void SetMode(int mode) override;

    void AddRigidBody(IRigidBody* body) override;
    void RemoveRigidBody(IRigidBody* body) override;

    void DebugDraw() override;

    void SetCollisionFilter(CollisionFilterCallback callback) override;

    btDiscreteDynamicsWorld* GetRawWorld() const { return m_DynamicsWorld.get(); }

    std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape) override;
    std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) override;
    std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) override;
    std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) override;
    std::shared_ptr<ICollisionShape> CreateCompoundShape() override;
    std::shared_ptr<ICollisionShape> CreateMeshShape(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) override;
    void AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child, const glm::vec3& pos, const glm::quat& rot) override;

    void AddConstraint(std::shared_ptr<IConstraint> constraint) override;
    void RemoveConstraint(std::shared_ptr<IConstraint> constraint) override;
    std::shared_ptr<IConstraint> CreatePoint2PointConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB) override;
    std::shared_ptr<IConstraint> CreateHingeConstraint(std::shared_ptr<IRigidBody> rbA, std::shared_ptr<IRigidBody> rbB, const glm::vec3& pivotInA, const glm::vec3& pivotInB, const glm::vec3& axisInA, const glm::vec3& axisInB) override;

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_CollisionConfig;
    std::unique_ptr<CustomCollisionDispatcher> m_Dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_OverlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_DynamicsWorld;

    BulletDebugDrawer* m_CurrentDebugDrawer = nullptr;
    std::unique_ptr<BulletDebugDrawer> m_OwnedDebugDrawer;
};
