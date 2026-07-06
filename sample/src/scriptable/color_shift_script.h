#pragma once
#include <axis_all.h>
#include <cmath>

class ColorShiftScript : public Scriptable
{
public:
    float speed = 1.0f;
    float time = 0.0f;

    void OnCreate() override
    {
        speed = 0.5f + static_cast<float>(rand() % 100) / 100.0f;
        time = static_cast<float>(rand() % 100) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        time += dt;
        glm::vec3 color(0.5f + 0.5f * sin(time * speed), 0.5f + 0.5f * sin(time * speed + 2.0f),
                        0.5f + 0.5f * sin(time * speed + 4.0f));

        if (HasComponent<MeshRendererComponent>())
        {
            auto& renderer = GetComponent<MeshRendererComponent>();
            renderer.color = glm::vec4(color, 1.0f);
        }

        if (HasComponent<MaterialComponent>())
        {
            auto& mat = GetComponent<MaterialComponent>();
            mat.desc.emission = color * 0.35f;
            mat.gpu.dirty = true;
        }
    }
};
