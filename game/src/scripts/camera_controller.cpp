#include <scripts/camera_controller.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <axis_component.h>
#include <axis_platform.h>

REGISTER_SCRIPT(CameraController)

void CameraController::OnUpdate(float dt)
{
    if (!HasComponent<PositionComponent>() || !HasComponent<CameraComponent>())
        return;

    auto& posComp = GetComponent<PositionComponent>();
    auto& camera = GetComponent<CameraComponent>();

    const auto& mouse = Get<IOHandler>().GetMouse();
    const auto& keyboard = Get<IOHandler>().GetKeyboard();

    if (mouse.GetCursorMode() == CursorMode::Locked || mouse.GetCursorMode() == CursorMode::LockedHidden)
    {
        m_Yaw += mouse.GetXOffset() * mouseSensitivity;
        m_Pitch += mouse.GetYOffset() * mouseSensitivity;

        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;
        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;
    }

    float scroll = mouse.GetScrollY();
    if (scroll != 0.0f)
    {
        camera.fov -= scroll;
        if (camera.fov < 1.0f)
            camera.fov = 1.0f;
        if (camera.fov > 45.0f)
            camera.fov = 45.0f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    glm::vec3 frontNormalized = glm::normalize(front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    if (HasComponent<RotationComponent>())
    {
        auto& rot = GetComponent<RotationComponent>();
        rot.value = glm::quatLookAt(frontNormalized, worldUp);
    }
    glm::vec3 right = glm::normalize(glm::cross(frontNormalized, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, frontNormalized));

    float velocity = moveSpeed * dt;
    if (keyboard.GetKey(Key::W))
        posComp.value += frontNormalized * velocity;
    if (keyboard.GetKey(Key::S))
        posComp.value -= frontNormalized * velocity;
    if (keyboard.GetKey(Key::A))
        posComp.value -= right * velocity;
    if (keyboard.GetKey(Key::D))
        posComp.value += right * velocity;

    if (keyboard.GetKey(Key::Space))
        posComp.value += worldUp * velocity;
    if (keyboard.GetKey(Key::LeftShift))
        posComp.value -= worldUp * velocity;

    camera.aspectRatio = (float)Get<IOHandler>().GetMonitorManager().GetWidth() /
                         (float)Get<IOHandler>().GetMonitorManager().GetHeight();

    camera.projectionMatrix =
        glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);

    camera.viewMatrix = glm::lookAt(posComp.value, posComp.value + frontNormalized, up);
}
