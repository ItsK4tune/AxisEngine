#pragma once

#include <ecs/unit/core_components.h>
#include <script/logic/scriptable.h>

class DefaultCameraController : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    void FocusOnEntity(entt::entity entity);
    void UpdateTransform(glm::vec3& position, glm::quat& rotation, const glm::vec3& front, const glm::vec3& up);

    float m_MouseSensitivity = 0.1f;
    float m_BaseMoveSpeed = 35.0f;
    float m_PanSpeed = 0.02f;
    float m_ZoomSpeed = 0.5f;

    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_Distance = 10.0f; // Distance from pivot when orbiting

    glm::vec3 m_Velocity = glm::vec3(0.0f);
    glm::vec3 m_Pivot = glm::vec3(0.0f);

    bool m_WasLooking = false;
    bool m_WasPanning = false;
    bool m_WasOrbiting = false;
    bool m_WasFPressed = false;
};
