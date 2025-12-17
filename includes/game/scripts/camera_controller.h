#pragma once
#include <engine/core/scriptable.h>

class CameraController : public Scriptable
{
public:
    float moveSpeed = 5.0f;
    
    float radius = 15.0f;
    float angle = 0.0f;
    float height = 10.0f;

    void OnUpdate(float dt) override;
};