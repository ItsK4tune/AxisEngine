#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <ecs/unit/core_components.h>
#include <platform/logic/io_handler.h>
#include <platform/interface/i_ui_input_capture.h>
#include <editor/editor_viewport.h>

#include <script/logic/default_camera_controller.h>
#include <script/logic/script_registry.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

void DefaultCameraController::OnCreate()
{
    SetRunWhenPaused(true);

    auto& events = EventManager::Instance();
    m_SelectionChangedSubId = events.Subscribe<EntitySelectionChangedEvent>(
        [this](const EntitySelectionChangedEvent& e) { m_SelectedEntity = static_cast<entt::entity>(e.entity); });
    m_FocusRequestedSubId = events.Subscribe<EntityFocusRequestedEvent>(
        [this](const EntityFocusRequestedEvent& e) { m_PendingFocusEntity = static_cast<entt::entity>(e.entity); });
}

void DefaultCameraController::OnDestroy()
{
    auto& events = EventManager::Instance();
    if (m_SelectionChangedSubId != -1)
    {
        events.Unsubscribe<EntitySelectionChangedEvent>(m_SelectionChangedSubId);
        m_SelectionChangedSubId = -1;
    }
    if (m_FocusRequestedSubId != -1)
    {
        events.Unsubscribe<EntityFocusRequestedEvent>(m_FocusRequestedSubId);
        m_FocusRequestedSubId = -1;
    }
}

void DefaultCameraController::OnUpdate(float)
{
    if (!HasComponent<PositionComponent>() || !HasComponent<CameraComponent>())
        return;

    auto& posComp = GetComponent<PositionComponent>();
    auto& camera = GetComponent<CameraComponent>();

    auto* io = Resolve<IOHandler>();
    if (!io)
        return;

    const auto& mouse = io->GetMouse();
    const auto& keyboard = io->GetKeyboard();
    AppConfig config = GetConfig();

    float delta = GetRealDeltaTime();

    // UI state
    const auto* uiCapture = Resolve<IUIInputCapture>();
    const auto* editorViewport = Resolve<EditorViewportState>();
    const bool hoveringViewport = editorViewport && editorViewport->rect.hovered;
    const bool hoveringPanel = uiCapture && uiCapture->WantsPointerInput() && !hoveringViewport;
    const bool typing = uiCapture && uiCapture->WantsTextInput();

    const bool editorCursor = mouse.IsEditorMode();
    bool isRMB = mouse.IsEditorButtonPressed(Mouse::Right);
    bool isMMB = mouse.IsEditorButtonPressed(Mouse::Middle);
    bool isAlt = keyboard.GetRawKey(Key::LeftAlt) || keyboard.GetRawKey(Key::RightAlt);
    bool isCtrl = keyboard.GetRawKey(Key::LeftControl) || keyboard.GetRawKey(Key::RightControl);
    bool isLMB = mouse.IsEditorButtonPressed(Mouse::Left);

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

    // DefaultCameraController is an editor tool. It never changes the cursor
    // away from Editor mode; releasing the gesture only resets its drag state.
    if (!editorCursor || (!looking && !panning && !orbiting))
    {
        m_WasLooking = false;
        m_WasPanning = false;
        m_WasOrbiting = false;
        if (!editorCursor)
            m_Velocity = glm::vec3(0.0f);
    }

    // Framing
    bool isF = keyboard.GetRawKey(Key::F);
    if (editorCursor && isF && !m_WasFPressed && !typing)
    {
        m_WasFPressed = true;
        entt::entity target = m_SelectedEntity;
        if (target != entt::null && GetScene().IsValid(target))
        {
            FocusOnEntity(target);
        }
    }
    if (!isF)
        m_WasFPressed = false;

    if (m_PendingFocusEntity != entt::null)
    {
        entt::entity target = m_PendingFocusEntity;
        if (target != entt::null && GetScene().IsValid(target))
        {
            FocusOnEntity(target);
        }
        m_PendingFocusEntity = entt::null;
    }

    // Movement multipliers
    float speedMultiplier = 1.0f;
    if (keyboard.GetRawKey(Key::LeftShift))
        speedMultiplier = 2.5f;
    if (keyboard.GetRawKey(Key::LeftControl))
        speedMultiplier = 0.25f;

    float scroll = mouse.GetEditorScrollY();
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

    float xOffset = mouse.GetEditorXOffset() * config.input.mouseSensitivityX;
    float yOffset = mouse.GetEditorYOffset() * config.input.mouseSensitivityY;
    if (config.input.mouseInvertX)
        xOffset = -xOffset;
    if (config.input.mouseInvertY)
        yOffset = -yOffset;

    glm::vec3 targetVelocity(0.0f);

    if (looking)
    {
        m_Yaw += xOffset;
        m_Pitch += yOffset;

        if (!typing && !isCtrl && !isAlt)
        {
            float moveVel = m_BaseMoveSpeed * speedMultiplier;
            if (keyboard.GetRawKey(Key::W) || keyboard.GetRawKey(Key::Up))
                targetVelocity += frontNormalized * moveVel;
            if (keyboard.GetRawKey(Key::S) || keyboard.GetRawKey(Key::Down))
                targetVelocity -= frontNormalized * moveVel;
            if (keyboard.GetRawKey(Key::A) || keyboard.GetRawKey(Key::Left))
                targetVelocity -= right * moveVel;
            if (keyboard.GetRawKey(Key::D) || keyboard.GetRawKey(Key::Right))
                targetVelocity += right * moveVel;
            if (keyboard.GetRawKey(Key::E) || keyboard.GetRawKey(Key::PageUp))
                targetVelocity += worldUp * moveVel;
            if (keyboard.GetRawKey(Key::Q) || keyboard.GetRawKey(Key::PageDown))
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
        entt::entity selected = m_SelectedEntity;
        if (selected != entt::null && GetScene().IsValid(selected))
        {
            if (auto* tr = GetScene().TryGetComponent<WorldTransformComponent>(selected))
            {
                glm::vec3 selPos = glm::vec3(tr->worldMatrix[3]);
                m_Pivot = selPos;
                pivotSet = true;
                if (!m_WasOrbitingBefore)
                {
                    m_Distance = glm::distance(posComp.value, m_Pivot);
                    if (m_Distance < 0.1f)
                        m_Distance = 0.1f;
                    m_WasOrbitingBefore = true;
                }
            }
        }
        if (!pivotSet && !m_WasOrbitingBefore)
        {
            m_Distance = glm::distance(posComp.value, m_Pivot);
            if (m_Distance < 0.1f)
                m_Distance = 0.1f;
            m_WasOrbitingBefore = true;
        }

        m_Yaw += xOffset;
        m_Pitch += yOffset;
    }
    else
    {
        m_WasOrbitingBefore = false;
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
    if (HasComponent<PositionComponent>() && GetScene().IsValid(entity) &&
        GetScene().HasAllComponents<WorldTransformComponent>(entity))
    {
        auto& targetTrans = GetScene().GetComponent<WorldTransformComponent>(entity);
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
