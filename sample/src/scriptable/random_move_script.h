#pragma once
#include <axis_all.h>
#include <cmath>

class RandomMoveScript : public Scriptable
{
public:
    float speed = 2.0f;
    float range = 10.0f;
    glm::vec3 startPos = glm::vec3(0.0f);
    glm::vec3 targetPos = glm::vec3(0.0f);
    float timer = 0.0f;
    float changeInterval = 2.0f;

    void OnCreate() override
    {
        if (HasComponent<PositionComponent>())
        {
            startPos = GetComponent<PositionComponent>().value;
            targetPos = startPos;
        }
        speed = 2.0f + static_cast<float>(rand() % 400) / 100.0f;
        changeInterval = 1.0f + static_cast<float>(rand() % 200) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        timer += dt;
        if (timer >= changeInterval)
        {
            timer = 0.0f;
            float dx = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * range;
            float dy = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * range;
            targetPos = startPos + glm::vec3(dx, dy, 0.0f);
        }

        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value = glm::mix(pos.value, targetPos, speed * dt);
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};
