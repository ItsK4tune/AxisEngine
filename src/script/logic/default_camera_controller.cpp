#include <ecs/unit/core_components.h>
#include <core/logic/application.h>
#include <platform/logic/io_handler.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <platform/logic/input_manager.h>
#include <script/logic/default_camera_controller.h>
#include <script/logic/script_registry.h>

REGISTER_SCRIPT(DefaultCameraController)

void DefaultCameraController::OnCreate()
{
    SetRunWhenPaused(true);
}

void DefaultCameraController::OnUpdate(float dt)
{
    if (!HasComponent<PositionComponent>() || !HasComponent<CameraComponent>())
        return;

    auto &posComp = GetComponent<PositionComponent>();
    auto &camera = GetComponent<CameraComponent>();

    const auto &mouse = GetIOHandler().GetMouse();
    const auto &keyboard = GetIOHandler().GetKeyboard();

    float delta = GetRealDeltaTime();

    CursorMode mode = mouse.GetCursorMode();
    bool canControl = true;

    if (canControl)
    {
        const AppConfig& config = GetConfig();
        
        float xOffset = mouse.GetXOffset() * config.mouseSensitivityX;
        float yOffset = mouse.GetYOffset() * config.mouseSensitivityY;

        if (config.mouseInvertX) xOffset = -xOffset;
        if (config.mouseInvertY) yOffset = -yOffset;

        camera.yaw += xOffset;
        camera.pitch += yOffset;

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
        if (camera.fov > 120.0f)
            camera.fov = 120.0f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));

    float velocity = moveSpeed * delta;

    float speed = velocity;
    if (keyboard.GetKey(Key::LeftShift))
        speed *= 2.0f;

    if (keyboard.GetKey(Key::W))
        posComp.value += camera.front * speed;
    if (keyboard.GetKey(Key::S))
        posComp.value -= camera.front * speed;
    if (keyboard.GetKey(Key::A))
        posComp.value -= camera.right * speed;
    if (keyboard.GetKey(Key::D))
        posComp.value += camera.right * speed;

    if (keyboard.GetKey(Key::Space))
        posComp.value += camera.worldUp * speed;
    if (keyboard.GetKey(Key::LeftControl))
        posComp.value -= camera.worldUp * speed;

    if (camera.aspectRatio <= 0.0f)
    {
        camera.aspectRatio = (float)GetIOHandler().GetMonitorManager().GetWidth() / (float)GetIOHandler().GetMonitorManager().GetHeight();
    }
    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
    camera.viewMatrix = glm::lookAt(posComp.value, posComp.value + camera.front, camera.up);
}
