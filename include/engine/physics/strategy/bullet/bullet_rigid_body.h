#pragma once

#include <physics/interface/i_collision_shape.h>
#include <physics/interface/i_rigid_body.h>
#include <btBulletDynamicsCommon.h>
#include <functional>
#include <iostream>
#include <memory>

class BulletRigidBody : public IRigidBody
{
public:
    BulletRigidBody(btRigidBody* body, std::shared_ptr<ICollisionShape> shape) : m_Body(body), m_Shape(shape)
    {
    }
    ~BulletRigidBody();

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

    void Activate(bool forceActivation = true) override;
    bool IsActive() const override;
    bool IsStatic() const override;
    bool IsKinematic() const override;
    bool IsTrigger() const override;

    void SetKinematic(bool isKinematic) override;
    void SetStatic(bool isStatic) override;
    void SetTrigger(bool isTrigger) override;
    void SetAlwaysActive(bool alwaysActive) override;

    void SetUserPointer(void* ptr) override;
    void* GetUserPointer() const override;

    void SetLinearFactor(const glm::vec3& factor) override;
    void SetAngularFactor(const glm::vec3& factor) override;
    void SetDamping(float linearDamping, float angularDamping) override;

    btRigidBody* GetRaw() const
    {
        return m_Body;
    }

private:
    btRigidBody* m_Body = nullptr;
    std::shared_ptr<ICollisionShape> m_Shape;
};
