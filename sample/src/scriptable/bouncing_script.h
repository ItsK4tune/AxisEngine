#pragma once
#include <axis_all.h>

class BouncingScript : public Scriptable
{
public:
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 minBound = glm::vec3(-30.0f, 1.0f, -30.0f);
    glm::vec3 maxBound = glm::vec3(30.0f, 20.0f, 30.0f);

    void OnCreate() override
    {
        velocity =
            glm::normalize(glm::vec3(static_cast<float>(rand() % 200 - 100), static_cast<float>(rand() % 200 - 100),
                                     static_cast<float>(rand() % 200 - 100))) *
            (3.0f + static_cast<float>(rand() % 10));
    }

    void OnUpdate(float dt) override
    {
        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value += velocity * dt;

            if (pos.value.x < minBound.x)
            {
                pos.value.x = minBound.x;
                velocity.x *= -1.0f;
            }
            if (pos.value.x > maxBound.x)
            {
                pos.value.x = maxBound.x;
                velocity.x *= -1.0f;
            }
            if (pos.value.y < minBound.y)
            {
                pos.value.y = minBound.y;
                velocity.y *= -1.0f;
            }
            if (pos.value.y > maxBound.y)
            {
                pos.value.y = maxBound.y;
                velocity.y *= -1.0f;
            }
            if (pos.value.z < minBound.z)
            {
                pos.value.z = minBound.z;
                velocity.z *= -1.0f;
            }
            if (pos.value.z > maxBound.z)
            {
                pos.value.z = maxBound.z;
                velocity.z *= -1.0f;
            }
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};
