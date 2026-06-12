#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

class IAudioListener
{
public:
    virtual ~IAudioListener() = default;

    virtual void SetPosition(const glm::vec3& pos) = 0;
    virtual glm::vec3 GetPosition() const = 0;

    virtual void SetOrientation(const glm::vec3& forward, const glm::vec3& up) = 0;
    virtual glm::vec3 GetForward() const = 0;
    virtual glm::vec3 GetUp() const = 0;

    virtual void SetVelocity(const glm::vec3& vel) = 0;
    virtual glm::vec3 GetVelocity() const = 0;
};
