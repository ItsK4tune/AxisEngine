#include <ecs/logic/streaming_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>

REGISTER_SYSTEM(StreamingSystem)

void StreamingSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<StreamingSystem>(this);
}

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
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    
    for (auto entity : view)
    {
        auto& stream = view.get<StreamingComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);
        float distSq = glm::distance2(camPos, pos.value);


        if (distSq < stream.loadDistance * stream.loadDistance)
        {
            if (!stream.isRequested) {
                LOGGER_INFO("StreamingSystem") << "Distance threshold met, requesting resource: " << stream.modelPath;
                resources.LoadModelAsync(stream.modelPath, stream.modelPath, stream.isStatic);
                stream.isRequested = true;
            }
        }
        else if (distSq > stream.unloadDistance * stream.unloadDistance)
        {
            if (stream.isRequested) {
                LOGGER_INFO("StreamingSystem") << "Unload distance reached, removing mesh: " << stream.modelPath;
                if (scene.registry.all_of<MeshRendererComponent>(entity)) {
                    scene.registry.remove<MeshRendererComponent>(entity);
                }
                stream.isRequested = false;
            }
        }


        if (scene.registry.all_of<LODComponent, MeshRendererComponent>(entity))
        {
            auto& lod = scene.registry.get<LODComponent>(entity);
            auto& mesh = scene.registry.get<MeshRendererComponent>(entity);
            
            size_t bestLod = 0;
            for (size_t i = 0; i < lod.lodDistancesSq.size(); ++i) {
                if (distSq > lod.lodDistancesSq[i]) {
                    bestLod = i + 1;
                } else {
                    break;
                }
            }
            
            if (bestLod < lod.lodModels.size() && lod.lodModels[bestLod]) {
                if (mesh.model != lod.lodModels[bestLod]) {
                    mesh.model = lod.lodModels[bestLod];
                }
            }
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
