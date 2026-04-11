#include <ecs/unit/core_components.h>
#include <core/app/application.h>
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

    auto* io = Resolve<IOHandler>();
    if (!io) return;

    const auto &mouse = io->GetMouse();
    const auto &keyboard = io->GetKeyboard();

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

        m_Yaw += xOffset;
        m_Pitch += yOffset;

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
        if (camera.fov > 120.0f)
            camera.fov = 120.0f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    
    glm::vec3 frontNormalized = glm::normalize(front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(frontNormalized, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, frontNormalized));

    if (HasComponent<RotationComponent>())
    {
        auto& rot = GetComponent<RotationComponent>();
        rot.value = glm::quatLookAt(frontNormalized, up);
    }

    float velocity = moveSpeed * delta;

    float speed = velocity;
    if (keyboard.GetKey(Key::LeftShift))
        speed *= 2.0f;

    if (keyboard.GetKey(Key::W))
        posComp.value += frontNormalized * speed;
    if (keyboard.GetKey(Key::S))
        posComp.value -= frontNormalized * speed;
    if (keyboard.GetKey(Key::A))
        posComp.value -= right * speed;
    if (keyboard.GetKey(Key::D))
        posComp.value += right * speed;

    if (keyboard.GetKey(Key::Space))
        posComp.value += worldUp * speed;
    if (keyboard.GetKey(Key::LeftControl))
        posComp.value -= worldUp * speed;

    camera.aspectRatio = (float)io->GetMonitorManager().GetWidth() / (float)io->GetMonitorManager().GetHeight();
    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
    camera.viewMatrix = glm::lookAt(posComp.value, posComp.value + frontNormalized, up);
}
