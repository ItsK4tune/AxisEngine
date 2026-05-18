#include <core/app/application.h>
#include <ecs/unit/core_components.h>
#include <platform/logic/io_handler.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <script/logic/default_camera_controller.h>
#include <platform/logic/input_manager.h>
#include <script/logic/script_registry.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#ifdef ENABLE_EDITOR
#include <editor/panels/scene_hierarchy_panel.h>
#include <imgui.h>

#endif

REGISTER_SCRIPT(DefaultCameraController)

void DefaultCameraController::OnCreate()
{
    SetRunWhenPaused(true);
}

void DefaultCameraController::OnUpdate(float dt)
{
    static bool s_WasOrbitingBefore = false;
    if (!HasComponent<PositionComponent>() || !HasComponent<CameraComponent>())
        return;

    auto& posComp = GetComponent<PositionComponent>();
    auto& camera = GetComponent<CameraComponent>();

    auto* io = Resolve<IOHandler>();
    if (!io)
        return;

    const auto& mouse = io->GetMouse();
    const auto& keyboard = io->GetKeyboard();
    const AppConfig& config = GetConfig();

    float delta = GetRealDeltaTime();

    // UI state
    bool hoveringPanel = false;
    bool typing = false;
#ifdef ENABLE_EDITOR
    hoveringPanel = ImGui::GetIO().WantCaptureMouse;
    typing = ImGui::GetIO().WantTextInput;
#endif

    bool isRMB = mouse.IsRightButtonPressed();
    bool isMMB = mouse.IsMiddleButtonPressed();
    bool isAlt = keyboard.GetKey(Key::LeftAlt) || keyboard.GetKey(Key::RightAlt);
    bool isCtrl = keyboard.GetKey(Key::LeftControl) || keyboard.GetKey(Key::RightControl);
    bool isLMB = mouse.IsLeftButtonPressed();

    bool looking = isRMB && !isAlt;
    bool panning = isMMB || (isRMB && isAlt);
    bool orbiting = isAlt && isLMB;

    if (looking && !m_WasLooking)
    {
        if (hoveringPanel)
            looking = false;
        else
            m_WasLooking = true;
    }
    if (panning && !m_WasPanning)
    {
        if (hoveringPanel)
            panning = false;
        else
            m_WasPanning = true;
    }
    if (orbiting && !m_WasOrbiting)
    {
        if (hoveringPanel)
            orbiting = false;
        else
            m_WasOrbiting = true;
    }

    looking = looking && m_WasLooking;
    panning = panning && m_WasPanning;
    orbiting = orbiting && m_WasOrbiting;

    // Manage cursor mode
    if (looking || panning || orbiting)
    {
        if (mouse.GetCursorMode() != CursorMode::LockedHidden)
        {
            io->GetMouse().SetCursorMode(CursorMode::LockedHidden);
        }
    }
    else
    {
        if (mouse.GetCursorMode() == CursorMode::LockedHidden)
        {
            io->GetMouse().SetCursorMode(CursorMode::Normal);
        }
        m_WasLooking = false;
        m_WasPanning = false;
        m_WasOrbiting = false;
    }

    // Framing
    bool isF = keyboard.GetKey(Key::F);
    if (isF && !m_WasFPressed && !typing)
    {
        m_WasFPressed = true;
#ifdef ENABLE_EDITOR
        entt::entity target = SceneHierarchyPanel::s_SelectedEntity;
        if (target != entt::null && GetScene().registry.valid(target))
        {
            FocusOnEntity(target);
        }
#endif
    }
    if (!isF)
        m_WasFPressed = false;

#ifdef ENABLE_EDITOR
    if (SceneHierarchyPanel::s_FocusRequested)
    {
        entt::entity target = SceneHierarchyPanel::s_FocusTargetEntity;
        if (target != entt::null && GetScene().registry.valid(target))
        {
            FocusOnEntity(target);
        }
        SceneHierarchyPanel::s_FocusRequested = false;
    }
#endif

    // Movement multipliers
    float speedMultiplier = 1.0f;
    if (keyboard.GetKey(Key::LeftShift))
        speedMultiplier = 2.5f;
    if (keyboard.GetKey(Key::LeftControl))
        speedMultiplier = 0.25f;

    float scroll = mouse.GetScrollY();
    if (looking && scroll != 0.0f)
    {
        m_BaseMoveSpeed += scroll;
        if (m_BaseMoveSpeed < 0.5f)
            m_BaseMoveSpeed = 0.5f;
        if (m_BaseMoveSpeed > 50.0f)
            m_BaseMoveSpeed = 50.0f;
    }

    // Build coordinate frame
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    glm::vec3 frontNormalized = glm::normalize(front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(frontNormalized, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, frontNormalized));

    float xOffset = mouse.GetXOffset() * m_MouseSensitivity;
    float yOffset = mouse.GetYOffset() * m_MouseSensitivity;
    if (config.mouseInvertX)
        xOffset = -xOffset;
    if (config.mouseInvertY)
        yOffset = -yOffset;

    glm::vec3 targetVelocity(0.0f);

    if (looking)
    {
        m_Yaw += xOffset;
        m_Pitch += yOffset;

        if (!typing && !isCtrl && !isAlt)
        {
            float moveVel = m_BaseMoveSpeed * speedMultiplier;
            if (keyboard.GetKey(Key::W) || keyboard.GetKey(Key::Up))
                targetVelocity += frontNormalized * moveVel;
            if (keyboard.GetKey(Key::S) || keyboard.GetKey(Key::Down))
                targetVelocity -= frontNormalized * moveVel;
            if (keyboard.GetKey(Key::A) || keyboard.GetKey(Key::Left))
                targetVelocity -= right * moveVel;
            if (keyboard.GetKey(Key::D) || keyboard.GetKey(Key::Right))
                targetVelocity += right * moveVel;
            if (keyboard.GetKey(Key::E) || keyboard.GetKey(Key::PageUp))
                targetVelocity += worldUp * moveVel;
            if (keyboard.GetKey(Key::Q) || keyboard.GetKey(Key::PageDown))
                targetVelocity -= worldUp * moveVel;
        }
    }
    else if (panning)
    {
        posComp.value -= right * (xOffset * m_PanSpeed * m_Distance * speedMultiplier);
        posComp.value -= up * (yOffset * m_PanSpeed * m_Distance * speedMultiplier);
        m_Pivot -= right * (xOffset * m_PanSpeed * m_Distance * speedMultiplier);
        m_Pivot -= up * (yOffset * m_PanSpeed * m_Distance * speedMultiplier);
    }
    else if (orbiting)
    {
        bool pivotSet = false;
#ifdef ENABLE_EDITOR
        entt::entity selected = SceneHierarchyPanel::s_SelectedEntity;
        if (selected != entt::null && GetScene().registry.valid(selected))
        {
            if (auto* tr = GetScene().registry.try_get<WorldTransformComponent>(selected))
            {
                glm::vec3 selPos = glm::vec3(tr->worldMatrix[3]);
                m_Pivot = selPos;
                pivotSet = true;
                if (!s_WasOrbitingBefore)
                {
                    m_Distance = glm::distance(posComp.value, m_Pivot);
                    if (m_Distance < 0.1f)
                        m_Distance = 0.1f;
                    s_WasOrbitingBefore = true;
                }
            }
        }
#endif
        if (!pivotSet && !s_WasOrbitingBefore)
        {
            m_Distance = glm::distance(posComp.value, m_Pivot);
            if (m_Distance < 0.1f)
                m_Distance = 0.1f;
            s_WasOrbitingBefore = true;
        }

        m_Yaw += xOffset;
        m_Pitch += yOffset;
    }
    else
    {
        s_WasOrbitingBefore = false;
    }

    // Apply Pitch clamp globally after input
    if (m_Pitch > 89.0f)
        m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
        m_Pitch = -89.0f;

    // Zoom (Dolly) without FOV change
    if (!looking && scroll != 0.0f && !hoveringPanel)
    {
        float zoomAmount = scroll * m_ZoomSpeed * speedMultiplier * std::max(1.0f, m_Distance * 0.2f);
        m_Distance -= zoomAmount;
        if (m_Distance < 0.1f)
            m_Distance = 0.1f;

        posComp.value += frontNormalized * zoomAmount;
        m_Pivot = posComp.value + (frontNormalized * m_Distance);
    }

    // If orbiting, force position to pivot distance
    if (orbiting)
    {
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        frontNormalized = glm::normalize(front);

        posComp.value = m_Pivot - (frontNormalized * m_Distance);
    }

    // Apply damped velocity to position
    float dampening = 15.0f;
    m_Velocity = glm::mix(m_Velocity, targetVelocity, glm::clamp(delta * dampening, 0.0f, 1.0f));

    if (!orbiting && !panning)
    {
        posComp.value += m_Velocity * delta;
        // If we move via WASD, sync pivot forward
        m_Pivot = posComp.value + (frontNormalized * m_Distance);
    }

    // Final orientation update
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    frontNormalized = glm::normalize(front);
    right = glm::normalize(glm::cross(frontNormalized, worldUp));
    up = glm::normalize(glm::cross(right, frontNormalized));

    if (HasComponent<RotationComponent>())
    {
        auto& rot = GetComponent<RotationComponent>();
        rot.value = glm::quatLookAt(frontNormalized, up);
    }

    camera.aspectRatio = (float)io->GetMonitorManager().GetWidth() / (float)io->GetMonitorManager().GetHeight();
    camera.projectionMatrix =
        glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
    camera.viewMatrix = glm::lookAt(posComp.value, posComp.value + frontNormalized, up);
}

void DefaultCameraController::FocusOnEntity(entt::entity entity)
{
    if (HasComponent<PositionComponent>() && GetScene().registry.valid(entity) &&
        GetScene().registry.all_of<WorldTransformComponent>(entity))
    {
        auto& targetTrans = GetScene().registry.get<WorldTransformComponent>(entity);
        glm::vec3 targetPos = targetTrans.worldMatrix[3];

        m_Pivot = targetPos;

        glm::vec3 front;
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        glm::vec3 frontNormalized = glm::normalize(front);

        m_Distance = 10.0f;
        auto& posComp = GetComponent<PositionComponent>();
        posComp.value = m_Pivot - (frontNormalized * m_Distance);
    }
}
