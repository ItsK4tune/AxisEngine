#include <physics/logic/physics_query_service.h>
#include <physics/interface/i_physics_world.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <ecs/unit/core_components.h>
#include <ecs/logic/entity_manager.h>
#include <scene/logic/scene.h>
#include <core/logic/service_locator.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>

PhysicsQueryService::PhysicsQueryService()
{
}

RayHit PhysicsQueryService::Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float distance)
{
    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    return physicsWorld ? physicsWorld->Raycast(origin, direction, distance) : RayHit{};
}

RayHit PhysicsQueryService::Raycast(const glm::vec3 &start, const glm::vec3 &end)
{
    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    if (!physicsWorld) return {};

    glm::vec3 dir = end - start;
    float dist = glm::length(dir);
    if (dist < 0.0001f) return {};

    return physicsWorld->Raycast(start, glm::normalize(dir), dist);
}

RayHit PhysicsQueryService::Raycast(const glm::vec3 &origin, float yaw, float pitch, float distance)
{
    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    return physicsWorld ? physicsWorld->Raycast(origin, RaycastUtils::AngleToDirection(yaw, pitch), distance) : RayHit{};
}

RayHit PhysicsQueryService::RaycastFromScreen(const glm::vec2 &screenPos, float distance)
{
    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    Scene* scene = ServiceLocator::Instance().Resolve<Scene>();
    if (!physicsWorld || !scene) return {};

    entt::entity camEntity = EntityManager::GetActiveCamera(*scene);
    if (camEntity == entt::null) return {};

    auto &camera = scene->registry.get<CameraComponent>(camEntity);
    auto &monitorManager = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager();
    glm::vec2 viewportSize(monitorManager.GetWidth(), monitorManager.GetHeight());

    Ray ray = RaycastUtils::CalculateRay(screenPos, viewportSize, camera.viewMatrix, camera.projectionMatrix);
    return physicsWorld->Raycast(ray.origin, ray.direction, distance);
}
