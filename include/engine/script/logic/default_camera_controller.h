#pragma once

#include <ecs/unit/core_components.h>
#include <script/logic/scriptable.h>

class DefaultCameraController : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;
    void OnDestroy() override;

private:
    void FocusOnEntity(entt::entity entity);

    float m_BaseMoveSpeed = 35.0f;
    float m_PanSpeed = 0.02f;
    float m_ZoomSpeed = 0.5f;

    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_Distance = 10.0f;  // Distance from pivot when orbiting

    glm::vec3 m_Velocity = glm::vec3(0.0f);
    glm::vec3 m_Pivot = glm::vec3(0.0f);
    entt::entity m_SelectedEntity = entt::null;
    entt::entity m_PendingFocusEntity = entt::null;

    bool m_WasLooking = false;
    bool m_WasPanning = false;
    bool m_WasOrbiting = false;
    bool m_WasOrbitingBefore = false;
    bool m_WasFPressed = false;

    int m_SelectionChangedSubId = -1;
    int m_FocusRequestedSubId = -1;
};
