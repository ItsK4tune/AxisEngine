#pragma once

#include <glm/glm.hpp>

class ICameraService
{
public:
    virtual ~ICameraService() = default;

    virtual glm::vec3 GetCameraPosition() const = 0;
    virtual glm::mat4 GetViewMatrix() const = 0;
    virtual glm::mat4 GetProjectionMatrix() const = 0;
    virtual float GetNearPlane() const = 0;
    virtual float GetFarPlane() const = 0;
};
