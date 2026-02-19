#pragma once

#include <interface/physics/i_physics_world.h>
#include <physic/backends/bullet_rigid_body.h>
#include <physic/backends/bullet_collision_shape.h>
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <memory>
#include <physic/backends/bullet_debug_drawer.h>

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

    btDiscreteDynamicsWorld* GetRawWorld() const { return m_DynamicsWorld.get(); }

    std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot, std::shared_ptr<ICollisionShape> shape) override;
    std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) override;
    std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) override;
    std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) override;
    std::shared_ptr<ICollisionShape> CreateCompoundShape() override;
    void AddChildShape(ICollisionShape* parent, ICollisionShape* child, const glm::vec3& pos, const glm::quat& rot) override;

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_CollisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_Dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_OverlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_DynamicsWorld;

    BulletDebugDrawer* m_CurrentDebugDrawer = nullptr;
    std::unique_ptr<BulletDebugDrawer> m_OwnedDebugDrawer;
};
