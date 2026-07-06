#pragma once
#include <axis_all.h>
#include <glm/gtx/quaternion.hpp>

class RotateScript : public Scriptable
{
public:
    glm::vec3 rotationSpeed = glm::vec3(0.0f);

    void OnCreate() override
    {
        rotationSpeed = glm::vec3(static_cast<float>(rand() % 360 - 180), static_cast<float>(rand() % 360 - 180),
                                  static_cast<float>(rand() % 360 - 180));
    }

    void OnUpdate(float dt) override
    {
        if (HasComponent<RotationComponent>())
        {
            auto& rot = GetComponent<RotationComponent>();
            glm::quat deltaRot = glm::quat(glm::radians(rotationSpeed * dt));
            rot.value = rot.value * deltaRot;
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};
