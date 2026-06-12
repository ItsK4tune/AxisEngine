#pragma once
#include <physics/interface/i_rigid_body.h>
#include <physics/interface/i_collision_shape.h>
#include <PxRigidActor.h>
#include <memory>

class PhysXRigidBody : public IRigidBody
{
public:
    PhysXRigidBody(physx::PxRigidActor* actor, std::shared_ptr<ICollisionShape> shape)
        : m_Actor(actor), m_Shape(shape)
    {
    }

    ~PhysXRigidBody() override
    {
        if (m_Actor)
        {
            m_Actor->release();
            m_Actor = nullptr;
        }
    }

    void SetLinearVelocity(const glm::vec3& vel) override;
    void SetAngularVelocity(const glm::vec3& vel) override;
    glm::vec3 GetLinearVelocity() const override;
    glm::vec3 GetAngularVelocity() const override;

    void ApplyCentralForce(const glm::vec3& force) override;
    void ApplyCentralImpulse(const glm::vec3& impulse) override;
    void ApplyTorque(const glm::vec3& torque) override;

    void SetFriction(float friction) override;
    void SetRestitution(float restitution) override;

    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override;
    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override;

    void Activate(bool forceActivation = false) override;
    bool IsActive() const override;
    bool IsStatic() const override;
    bool IsKinematic() const override;
    bool IsTrigger() const override;

    void SetKinematic(bool isKinematic) override;
    void SetStatic(bool isStatic) override;
    void SetTrigger(bool isTrigger) override;
    void SetAlwaysActive(bool alwaysActive) override;

    void SetUserPointer(void* ptr) override
    {
        if (m_Actor)
            m_Actor->userData = ptr;
    }

    void* GetUserPointer() const override
    {
        return m_Actor ? m_Actor->userData : nullptr;
    }

    void SetLinearFactor(const glm::vec3& factor) override;
    void SetAngularFactor(const glm::vec3& factor) override;
    void SetDamping(float linearDamping, float angularDamping) override;

    physx::PxRigidActor* GetRaw() const { return m_Actor; }
    std::shared_ptr<ICollisionShape> GetShape() const { return m_Shape; }

private:
    physx::PxRigidActor* m_Actor = nullptr;
    std::shared_ptr<ICollisionShape> m_Shape;
};
