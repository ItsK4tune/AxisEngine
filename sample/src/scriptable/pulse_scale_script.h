#pragma once
#include <axis_all.h>
#include <cmath>

class PulseScaleScript : public Scriptable
{
public:
    float speed = 2.0f;
    float time = 0.0f;

    void OnCreate() override
    {
        speed = 1.0f + static_cast<float>(rand() % 200) / 100.0f;
        time = static_cast<float>(rand() % 100) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        time += dt;
        if (HasComponent<ScaleComponent>())
        {
            auto& scale = GetComponent<ScaleComponent>();
            float s = 1.0f + 0.4f * sin(time * speed);
            scale.value = glm::vec3(s);
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};
