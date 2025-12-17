#include <game/scripts/camera_controller.h>
#include <engine/core/application.h>
#include <glm/gtc/matrix_transform.hpp>

void CameraController::OnUpdate(float dt)
{
    if (!HasComponent<TransformComponent>() || !HasComponent<CameraComponent>()) return;

    auto &transform = GetComponent<TransformComponent>();
    auto &camera = GetComponent<CameraComponent>();
    const auto &keyboard = m_App->GetKeyboard();

    // 1. Zoom (Radius) - W/S
    if (keyboard.GetKey(GLFW_KEY_W)) radius -= moveSpeed * dt;
    if (keyboard.GetKey(GLFW_KEY_S)) radius += moveSpeed * dt;
    if (radius < 2.0f) radius = 2.0f; // Min zoom
    if (radius > 50.0f) radius = 50.0f; // Max zoom

    // 2. Rotate (Angle) - A/D
    if (keyboard.GetKey(GLFW_KEY_A)) angle -= moveSpeed * dt;
    if (keyboard.GetKey(GLFW_KEY_D)) angle += moveSpeed * dt;

    // 3. Height (Y) - Space/Shift
    if (keyboard.GetKey(GLFW_KEY_SPACE)) height += moveSpeed * dt;
    if (keyboard.GetKey(GLFW_KEY_LEFT_SHIFT)) height -= moveSpeed * dt;

    // 4. Tính toán vị trí mới (Polar Coordinates)
    float camX = sin(angle) * radius;
    float camZ = cos(angle) * radius;
    
    transform.position = glm::vec3(camX, height, camZ);

    // 5. Look At Center (0,0,0)
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    
    // Cập nhật ma trận View
    camera.viewMatrix = glm::lookAt(transform.position, target, glm::vec3(0,1,0));
    
    // Cập nhật Projection
    camera.aspectRatio = (float)m_App->GetWidth() / (float)m_App->GetHeight();
    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);

    // Cập nhật vectors hướng để dùng cho mục đích khác nếu cần (dù viewMatrix đã tính rồi)
    camera.front = glm::normalize(target - transform.position);
    camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0,1,0)));
    camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
}