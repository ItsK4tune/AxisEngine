#pragma once

#include <axis_build.h>

class CameraController : public Scriptable
{
public:
    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.1f;

    void OnUpdate(float dt) override;

private:
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
};
