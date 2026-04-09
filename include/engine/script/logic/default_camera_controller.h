#pragma once

#include <ecs/unit/core_components.h>
#include <script/logic/scriptable.h>

class DefaultCameraController : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    float mouseSensitivity = 0.1f;
    float moveSpeed = 10.0f;
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
};
