#pragma once
#include <engine/core/scriptable.h>

class CameraController : public Scriptable
{
public:
    float radius = 15.0f;
    float minRadius = 3.0f;
    float maxRadius = 60.0f;

    float yaw   = 0.0f;
    float pitch = 0.4f;

    float rotateSpeed = 1.8f;
    float zoomSpeed   = 20.0f;
    float panSpeed    = 0.02f;

    glm::vec3 pivot = glm::vec3(0.0f);

    void OnUpdate(float dt) override;
};