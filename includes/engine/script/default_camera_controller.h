#pragma once

#include <ecs/component.h>
#include <script/scriptable.h>

class DefaultCameraController : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    float mouseSensitivity = 0.1f;
    float moveSpeed = 10.0f;
};
