#pragma once
#include <axis_all.h>
#include <cmath>

class OrbitScript : public Scriptable
{
public:
    float speed = 1.0f;
    float radius = 5.0f;
    glm::vec3 center = glm::vec3(0.0f);
    float angle = 0.0f;

    void OnCreate() override
    {
        angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        speed = 0.5f + static_cast<float>(rand() % 100) / 100.0f;
        radius = 5.0f + static_cast<float>(rand() % 100) / 10.0f;
        if (HasComponent<PositionComponent>())
        {
            center = GetComponent<PositionComponent>().value;
        }
    }

    void OnUpdate(float dt) override
    {
        angle += speed * dt;
        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value = center + glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
            MarkTransformDirty();
        }
    }
};
