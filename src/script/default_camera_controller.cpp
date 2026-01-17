#include <script/default_camera_controller.h>
#include <glm/gtc/matrix_transform.hpp>
#include <app/application.h>
#include <script/script_registry.h>

REGISTER_SCRIPT(DefaultCameraController)

void DefaultCameraController::OnCreate()
{
    SetRunWhenPaused(true);
}

void DefaultCameraController::OnUpdate(float dt)
{
    if (!HasComponent<TransformComponent>() || !HasComponent<CameraComponent>())
        return;

    auto &transform = GetComponent<TransformComponent>();
    auto &camera = GetComponent<CameraComponent>();

    const auto &mouse = GetAppHandler().GetMouse();
    const auto &keyboard = GetAppHandler().GetKeyboard();

    // Use RealDeltaTime to remain independent of TimeScale (SlowMo)
    float delta = GetRealDeltaTime();
    
    // Check for any locked/captured mode to enable camera control
    CursorMode mode = mouse.GetCursorMode();
    bool canControl = (mode == CursorMode::Locked) || 
                      (mode == CursorMode::LockedCenter) ||
                      (mode == CursorMode::LockedHidden) ||
                      (mode == CursorMode::LockedHiddenCenter);

    if (canControl)
    {
        camera.yaw += mouse.GetXOffset() * mouseSensitivity;
        camera.pitch += mouse.GetYOffset() * mouseSensitivity;

        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;
    }

    // Zoom
    float scroll = mouse.GetScrollY();
    if (scroll != 0.0f)
    {
        camera.fov -= scroll;
        if (camera.fov < 1.0f)
            camera.fov = 1.0f;
        if (camera.fov > 120.0f)
            camera.fov = 120.0f;
    }

    // Vectors
    glm::vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));

    // Movement (WASD + Space/Shift for Up/Down)
    float velocity = moveSpeed * delta;
    
    float speed = velocity;
    if (keyboard.GetKey(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
    
    if (keyboard.GetKey(GLFW_KEY_W))
        transform.position += camera.front * speed;
    if (keyboard.GetKey(GLFW_KEY_S))
        transform.position -= camera.front * speed;
    if (keyboard.GetKey(GLFW_KEY_A))
        transform.position -= camera.right * speed;
    if (keyboard.GetKey(GLFW_KEY_D))
        transform.position += camera.right * speed;

    if (keyboard.GetKey(GLFW_KEY_SPACE))
        transform.position += camera.worldUp * speed;
    if (keyboard.GetKey(GLFW_KEY_LEFT_CONTROL))
        transform.position -= camera.worldUp * speed;

    // Updates
    camera.aspectRatio = (float)m_App->GetWidth() / (float)m_App->GetHeight();
    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
    camera.viewMatrix = glm::lookAt(transform.position, transform.position + camera.front, camera.up);
}
