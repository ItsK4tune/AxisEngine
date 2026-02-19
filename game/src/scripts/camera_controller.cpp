#include <scripts/camera_controller.h>
#include <glm/gtc/matrix_transform.hpp>
#include <app/application.h>
#include <script/script_registry.h>

REGISTER_SCRIPT(CameraController)

void CameraController::OnUpdate(float dt)
{
    if (!HasComponent<TransformComponent>() || !HasComponent<CameraComponent>())
        return;

    auto &transform = GetComponent<TransformComponent>();
    auto &camera = GetComponent<CameraComponent>();

    const auto &mouse = GetIOHandler().GetMouse();
    const auto &keyboard = GetIOHandler().GetKeyboard();

    if (mouse.GetCursorMode() == Input::CursorMode::Locked || mouse.GetCursorMode() == Input::CursorMode::LockedHidden)
    {
        camera.yaw += mouse.GetXOffset() * mouseSensitivity;
        camera.pitch += mouse.GetYOffset() * mouseSensitivity;

        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;
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
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));

    float velocity = moveSpeed * dt;
    if (keyboard.GetKey(Input::Key::W))
        transform.position += camera.front * velocity;
    if (keyboard.GetKey(Input::Key::S))
        transform.position -= camera.front * velocity;
    if (keyboard.GetKey(Input::Key::A))
        transform.position -= camera.right * velocity;
    if (keyboard.GetKey(Input::Key::D))
        transform.position += camera.right * velocity;

    if (keyboard.GetKey(Input::Key::Space))
        transform.position += camera.worldUp * velocity;
    if (keyboard.GetKey(Input::Key::LeftShift))
        transform.position -= camera.worldUp * velocity;

    camera.aspectRatio = (float)m_App->GetWidth() / (float)m_App->GetHeight();

    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);

    camera.viewMatrix = glm::lookAt(transform.position, transform.position + camera.front, camera.up);
}