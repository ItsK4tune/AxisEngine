#include <game/scripts/camera_controller.h>
#include <engine/core/application.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

void CameraController::OnUpdate(float dt)
{
    if (!HasComponent<TransformComponent>() || !HasComponent<CameraComponent>())
        return;

    auto &transform = GetComponent<TransformComponent>();
    auto &camera = GetComponent<CameraComponent>();

    const auto &keyboard = m_App->GetKeyboard();
    const auto &mouse = m_App->GetMouse();

    if (keyboard.GetKey(GLFW_KEY_A))
        yaw -= rotateSpeed * dt;

    if (keyboard.GetKey(GLFW_KEY_D))
        yaw += rotateSpeed * dt;

    if (keyboard.GetKey(GLFW_KEY_W))
        pitch += rotateSpeed * dt;

    if (keyboard.GetKey(GLFW_KEY_S))
        pitch -= rotateSpeed * dt;

    pitch = glm::clamp(pitch, -1.4f, 1.4f);

    if (keyboard.GetKey(GLFW_KEY_Q))
        radius -= zoomSpeed * dt;

    if (keyboard.GetKey(GLFW_KEY_E))
        radius += zoomSpeed * dt;

    radius -= mouse.GetScrollY() * zoomSpeed * 0.1f;
    radius = glm::clamp(radius, minRadius, maxRadius);

    if (mouse.IsRightButtonPressed())
    {
        float dx = mouse.GetXOffset();
        float dy = mouse.GetYOffset();

        glm::vec3 forward = glm::normalize(glm::vec3(
            std::sin(yaw), 0.0f, std::cos(yaw)));

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));

        pivot += right * dx * panSpeed;
        pivot += forward * dy * panSpeed;
    }

    glm::vec3 offset;
    offset.x = std::cos(pitch) * std::sin(yaw) * radius;
    offset.y = std::sin(pitch) * radius;
    offset.z = std::cos(pitch) * std::cos(yaw) * radius;

    transform.position = pivot + offset;

    camera.viewMatrix = glm::lookAt(
        transform.position,
        pivot,
        glm::vec3(0, 1, 0));

    camera.aspectRatio =
        (float)m_App->GetWidth() / (float)m_App->GetHeight();

    camera.projectionMatrix = glm::perspective(
        glm::radians(camera.fov),
        camera.aspectRatio,
        camera.nearPlane,
        camera.farPlane);

    camera.front = glm::normalize(pivot - transform.position);
    camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0, 1, 0)));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}
