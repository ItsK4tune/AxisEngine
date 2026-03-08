#pragma once

#include <script/logic/scriptable.h>

class CameraController : public Scriptable
{
public:
    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.1f;

    void OnUpdate(float dt) override;
};
