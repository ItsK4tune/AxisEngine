#pragma once

#include <render/type/graphics_types.h>
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
    virtual void UploadCameraUBO(const GPUCameraData& camData) = 0;
    virtual void RestoreCameraState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane,
                                    float farPlane) = 0;
};
