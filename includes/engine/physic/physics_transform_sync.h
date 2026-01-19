#pragma once

#include <entt/entt.hpp>

class Scene;
class PhysicsWorld;

class PhysicsTransformSync
{
public:
    PhysicsTransformSync(Scene& scene, PhysicsWorld& physics);
    ~PhysicsTransformSync();

    void SyncTransformToPhysics(entt::entity entity);
    void SyncPhysicsToTransform(entt::entity entity);
    
    void SyncAllTransformsToPhysics();
    void SyncAllPhysicsToTransforms();
    
    void SetupPhysicsConstraints(entt::entity entity);
    void RemovePhysicsConstraints(entt::entity entity);

private:
    Scene& m_Scene;
    PhysicsWorld& m_Physics;
};
