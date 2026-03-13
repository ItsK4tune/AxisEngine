#include <ecs/logic/streaming_system.h>
#include <ecs/manager/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <core/logic/logger.h>

void StreamingSystem::Update(Scene& scene, float dt)
{
    m_Timer += dt;
    if (m_Timer < m_CheckInterval) return;
    m_Timer = 0.0f;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    if (!camPosComp) return;
    glm::vec3 camPos = camPosComp->value;

    auto view = scene.registry.view<StreamingComponent, PositionComponent>();
    for (auto entity : view)
    {
        auto& stream = view.get<StreamingComponent>(entity);
        if (stream.isRequested) continue;

        auto& pos = view.get<PositionComponent>(entity);
        float distSq = glm::distance2(camPos, pos.value);

        if (distSq < stream.loadDistance * stream.loadDistance)
        {
            LOGGER_INFO("StreamingSystem") << "Distance threshold met, requesting resource: " << stream.modelPath;
            m_Ctx.resources->LoadModelAsync(stream.modelPath, stream.modelPath, stream.isStatic);
            stream.isRequested = true;
        }
    }
}

std::vector<entt::id_type> StreamingSystem::GetReadComponents() const
{
    return {
        entt::type_id<StreamingComponent>().hash(),
        entt::type_id<PositionComponent>().hash()
    };
}

std::vector<entt::id_type> StreamingSystem::GetWriteComponents() const
{
    return {
        entt::type_id<StreamingComponent>().hash()
    };
}
