#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL

class ICharacterController
{
public:
    virtual ~ICharacterController() = default;

    virtual void SetWalkDirection(const glm::vec3& dir) = 0;
    virtual void SetVelocity(const glm::vec3& vel) = 0;
    virtual void Jump() = 0;
    virtual bool OnGround() const = 0;

    virtual void SetStepHeight(float height) = 0;
    virtual void SetMaxSlope(float slopeRadians) = 0;

    virtual void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const = 0;
    virtual void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) = 0;
    
    virtual void Activate(bool forceActivation = false) = 0;

    virtual void SetUserPointer(void* ptr) = 0;
    virtual void* GetUserPointer() const = 0;
};