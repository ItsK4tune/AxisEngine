#include <ecs/logic/camera_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

REGISTER_SYSTEM(CameraSystem)

void CameraSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<CameraSystem>(this);
    EventSystem::Instance().Subscribe<WindowResizedEvent>([this](const WindowResizedEvent& e) {
        auto* scene = ServiceLocator::Instance().Resolve<Scene>();
        if (!scene) return;

        auto view = scene->registry.view<CameraComponent>();
        float aspect = (float)e.width / (float)e.height;
        for (auto entity : view) {
            auto& camera = view.get<CameraComponent>(entity);
            camera.aspectRatio = aspect;
            camera.screenWidth = e.width;
            camera.screenHeight = e.height;
            if (!camera.isOrthographic) {
                camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), aspect, camera.nearPlane, camera.farPlane);
            } else {
                float h = camera.orthoSize;
                float w = h * aspect;
                camera.projectionMatrix = glm::ortho(-w, w, -h, h, camera.nearPlane, camera.farPlane);
            }
        }
    });
}

void CameraSystem::Shutdown()
{
}

void CameraSystem::Update(Scene& scene, float dt)
{
    auto view = scene.registry.view<CameraComponent, PositionComponent, RotationComponent>();
    for (auto entity : view) {
        auto& camera = view.get<CameraComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);
        auto& rot = view.get<RotationComponent>(entity);

        // Update View Matrix
        glm::vec3 front = rot.value * glm::vec3(0, 0, -1);
        glm::vec3 up = rot.value * glm::vec3(0, 1, 0);
        camera.viewMatrix = glm::lookAt(pos.value, pos.value + front, up);

        // Update Projection Matrix (Sync if not already done by WindowResized)
        if (camera.screenWidth > 0 && camera.screenHeight > 0) {
            float aspect = (float)camera.screenWidth / (float)camera.screenHeight;
            if (camera.aspectRatio <= 0.0f) camera.aspectRatio = aspect;
            
            if (!camera.isOrthographic) {
                camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
            } else {
                float h = camera.orthoSize;
                float w = h * camera.aspectRatio;
                camera.projectionMatrix = glm::ortho(-w, w, -h, h, camera.nearPlane, camera.farPlane);
            }
        }
    }
}

void CameraSystem::FixedUpdate(Scene& scene, float dt)
{
}

std::vector<entt::id_type> CameraSystem::GetReadComponents() const
{
    return {
        entt::type_id<CameraComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash()
    };
}

std::vector<entt::id_type> CameraSystem::GetWriteComponents() const
{
    return {
        entt::type_id<CameraComponent>().hash()
    };
}
