#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL

class IRigidBody
{
public:
    virtual ~IRigidBody() = default;

    virtual void SetLinearVelocity(const glm::vec3& vel) = 0;
    virtual void SetAngularVelocity(const glm::vec3& vel) = 0;

    virtual glm::vec3 GetLinearVelocity() const = 0;
    virtual glm::vec3 GetAngularVelocity() const = 0;

    virtual void ApplyCentralForce(const glm::vec3& force) = 0;
    virtual void ApplyTorque(const glm::vec3& torque) = 0;

    virtual void SetFriction(float friction) = 0;
    virtual void SetRestitution(float restitution) = 0;

    virtual void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) = 0;
    virtual void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const = 0;

    virtual void Activate(bool forceActivation = false) = 0;
    virtual bool IsActive() const = 0;

    virtual bool IsStatic() const = 0;
    virtual bool IsKinematic() const = 0;
    virtual bool IsTrigger() const = 0;

    virtual void SetKinematic(bool isKinematic) = 0;
    virtual void SetStatic(bool isStatic) = 0;
    virtual void SetAlwaysActive(bool alwaysActive) = 0;

    virtual void SetUserPointer(void* ptr) = 0;
    virtual void* GetUserPointer() const = 0;

    virtual void SetLinearFactor(const glm::vec3& factor) = 0;
    virtual void SetAngularFactor(const glm::vec3& factor) = 0;
};