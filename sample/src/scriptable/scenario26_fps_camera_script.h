#pragma once
#include <axis_all.h>
#include <glm/gtx/quaternion.hpp>
#include <cmath>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#endif

class Scenario26FpsCameraScript : public Scriptable
{
public:
    inline static float s_Yaw = -90.0f;
    inline static float s_Pitch = -8.0f;

    void OnCreate() override
    {
        s_Yaw = -90.0f;
        s_Pitch = -8.0f;
    }

    void OnUpdate(float dt) override
    {
        (void)dt;
        if (!HasComponent<PositionComponent>() || !HasComponent<RotationComponent>())
            return;

        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        if (!io)
            return;

        if (m_Target == entt::null || !GetScene().IsValid(m_Target))
        {
            auto view = GetScene().View<InfoComponent>();
            for (auto entity : view)
            {
                if (view.get<InfoComponent>(entity).name == "S26_CharacterController")
                {
                    m_Target = entity;
                    break;
                }
            }
        }

        if (m_Target == entt::null || !GetScene().IsValid(m_Target))
            return;

        bool mouseCaptured = false;
#ifdef ENABLE_EDITOR
        if (ImGui::GetCurrentContext())
            mouseCaptured = ImGui::GetIO().WantCaptureMouse;
#endif

        auto& mouse = io->GetMouse();
        const bool looking = mouse.IsRightButtonPressed() && !mouseCaptured;
        if (looking)
        {
            if (mouse.GetCursorMode() != CursorMode::LockedHidden)
                mouse.SetCursorMode(CursorMode::LockedHidden);
            s_Yaw += mouse.GetXOffset() * m_MouseSensitivity;
            s_Pitch += mouse.GetYOffset() * m_MouseSensitivity;
            s_Pitch = glm::clamp(s_Pitch, -82.0f, 82.0f);
        }
        else if (mouse.GetCursorMode() == CursorMode::LockedHidden)
        {
            mouse.SetCursorMode(CursorMode::Normal);
        }

        auto& targetPos = GetScene().GetComponent<PositionComponent>(m_Target);
        glm::vec3 front;
        front.x = std::cos(glm::radians(s_Yaw)) * std::cos(glm::radians(s_Pitch));
        front.y = std::sin(glm::radians(s_Pitch));
        front.z = std::sin(glm::radians(s_Yaw)) * std::cos(glm::radians(s_Pitch));
        front = glm::normalize(front);

        GetComponent<PositionComponent>().value = targetPos.value + glm::vec3(0.0f, 1.45f, 0.0f);
        GetComponent<RotationComponent>().value = glm::quatLookAt(front, glm::vec3(0.0f, 1.0f, 0.0f));
        if (HasComponent<WorldTransformComponent>())
            GetComponent<WorldTransformComponent>().isDirty = true;
    }

private:
    entt::entity m_Target = entt::null;
    float m_MouseSensitivity = 0.1f;
};
