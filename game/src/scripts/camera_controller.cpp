#include <scripts/camera_controller.h>
#include <core/app/application.h>
#include <systems/window/io_handler.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <systems/input/keyboard_manager.h>
#include <systems/input/mouse_manager.h>
#include <core/scripting/script_registry.h>

REGISTER_SCRIPT(CameraController)

void CameraController::OnUpdate(float dt)
{
    if (!HasComponent<PositionComponent>() || !HasComponent<CameraComponent>())
        return;

    auto &posComp = GetComponent<PositionComponent>();
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
        posComp.value += camera.front * velocity;
    if (keyboard.GetKey(Input::Key::S))
        posComp.value -= camera.front * velocity;
    if (keyboard.GetKey(Input::Key::A))
        posComp.value -= camera.right * velocity;
    if (keyboard.GetKey(Input::Key::D))
        posComp.value += camera.right * velocity;

    if (keyboard.GetKey(Input::Key::Space))
        posComp.value += camera.worldUp * velocity;
    if (keyboard.GetKey(Input::Key::LeftShift))
        posComp.value -= camera.worldUp * velocity;

    camera.aspectRatio = (float)m_Ctx.io->GetMonitorManager().GetWidth() / (float)m_Ctx.io->GetMonitorManager().GetHeight();

    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);

    camera.viewMatrix = glm::lookAt(posComp.value, posComp.value + camera.front, camera.up);
}
