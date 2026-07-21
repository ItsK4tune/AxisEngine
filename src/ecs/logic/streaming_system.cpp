#include <ecs/logic/streaming_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>

#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <resource/logic/resource_manager.h>
#include <resource/type/resource_events.h>
#include <glm/gtx/norm.hpp>


void StreamingSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<StreamingSystem>(this);
    m_EventSubscriptions.Clear();
    m_EventSubscriptions.Add(EventManager::Instance().Subscribe<ResourceLoadedEvent>([this](const auto& event) {
        if (event.type != "MODEL")
            return;
        if (event.success)
            m_FailedResources.erase(event.name);
        else
            m_FailedResources.insert(event.name);
    }));
}

void StreamingSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    SetUpdateThrottling(config.streamingUpdateThrottlingEnabled, config.streamingCheckIntervalSeconds);
}

void StreamingSystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    auto& services = ServiceLocator::Instance();
    auto* scene = services.Resolve<Scene>();
    auto* resources = services.Resolve<ResourceManager>();
    if (scene)
    {
        for (const auto& [entity, resourceName] : m_TrackedEntities)
        {
            if (!scene->IsValid(entity))
                continue;
            if (auto* mesh = scene->TryGetComponent<MeshRendererComponent>(entity);
                mesh && mesh->modelName == resourceName)
            {
                mesh->model.reset();
                mesh->modelName.clear();
            }
            if (auto* stream = scene->TryGetComponent<StreamingComponent>(entity))
            {
                stream->isRequested = false;
                stream->isResident = false;
                stream->state = StreamingState::Unloaded;
            }
        }
        scene->SetOctreeDirty(true);
    }
    if (resources)
    {
        for (const auto& [resourceName, residency] : m_Residencies)
            resources->UnloadModel(resourceName);
    }
    m_TrackedEntities.clear();
    m_Residencies.clear();
    m_FailedResources.clear();
    m_Timer = 0.0f;
}

std::string StreamingSystem::MakeResourceName(const std::string& modelPath, bool isStatic)
{
    return std::string("__axis_streaming__:") + (isStatic ? "static:" : "dynamic:") + modelPath;
}

void StreamingSystem::Release(entt::entity entity, ResourceManager& resources)
{
    const auto tracked = m_TrackedEntities.find(entity);
    if (tracked == m_TrackedEntities.end())
        return;

    const std::string resourceName = tracked->second;
    m_TrackedEntities.erase(tracked);
    const auto residency = m_Residencies.find(resourceName);
    if (residency == m_Residencies.end())
        return;
    if (residency->second.references > 0)
        --residency->second.references;
    if (residency->second.references == 0)
    {
        resources.UnloadModel(resourceName);
        m_Residencies.erase(residency);
    }
}

void StreamingSystem::Update(Scene& scene, float dt)
{
    if (m_UpdateThrottlingEnabled)
    {
        m_Timer += dt;
        if (m_Timer < m_CheckInterval)
            return;
        m_Timer = 0.0f;
    }

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto* camPosComp = scene.TryGetComponent<PositionComponent>(camEntity);
    auto* camWorld = scene.TryGetComponent<WorldTransformComponent>(camEntity);
    if (!camPosComp && !camWorld)
        return;
    const auto* camHierarchy = scene.TryGetComponent<HierarchyComponent>(camEntity);
    const bool cameraIsRoot = !camHierarchy || camHierarchy->parent == entt::null;
    const glm::vec3 camPos = cameraIsRoot && camPosComp
                                 ? camPosComp->value
                                 : (camWorld ? glm::vec3(camWorld->worldMatrix[3]) : camPosComp->value);

    auto view = scene.View<StreamingComponent, PositionComponent>();
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    const auto detachModel = [&scene](entt::entity entity, const std::string& resourceName) {
        if (auto* mesh = scene.TryGetComponent<MeshRendererComponent>(entity);
            mesh && mesh->modelName == resourceName)
        {
            mesh->model.reset();
            mesh->modelName.clear();
            scene.MarkOctreeEntityDirty(entity);
        }
    };

    for (auto tracked = m_TrackedEntities.begin(); tracked != m_TrackedEntities.end();)
    {
        if (!scene.IsValid(tracked->first) || !scene.HasAllComponents<StreamingComponent>(tracked->first))
        {
            const entt::entity staleEntity = tracked->first;
            if (scene.IsValid(staleEntity))
                detachModel(staleEntity, tracked->second);
            ++tracked;
            Release(staleEntity, resources);
        }
        else
        {
            ++tracked;
        }
    }

    for (auto entity : view)
    {
        auto& stream = view.get<StreamingComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);
        const auto* world = scene.TryGetComponent<WorldTransformComponent>(entity);
        const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(entity);
        const bool entityIsRoot = !hierarchy || hierarchy->parent == entt::null;
        const glm::vec3 entityPosition = !entityIsRoot && world ? glm::vec3(world->worldMatrix[3]) : pos.value;
        const float distSq = glm::distance2(camPos, entityPosition);
        const float loadDistance = (std::max)(0.0f, stream.loadDistance);
        const float unloadDistance = (std::max)(loadDistance, stream.unloadDistance);
        const std::string resourceName = MakeResourceName(stream.modelPath, stream.isStatic);

        auto tracked = m_TrackedEntities.find(entity);
        if (stream.isRequested && tracked == m_TrackedEntities.end())
        {
            stream.isRequested = false;
            stream.isResident = false;
            stream.state = StreamingState::Unloaded;
        }
        if (tracked != m_TrackedEntities.end() && tracked->second != resourceName)
        {
            detachModel(entity, tracked->second);
            Release(entity, resources);
            stream.isRequested = false;
            stream.isResident = false;
            stream.state = StreamingState::Unloaded;
        }

        const auto* info = scene.TryGetComponent<InfoComponent>(entity);
        const bool canStream = !info || info->isActive;
        if (canStream && !stream.modelPath.empty() && distSq < loadDistance * loadDistance)
        {
            if (!stream.isRequested)
            {
                LOGGER_INFO("StreamingSystem") << "Distance threshold met, requesting resource: " << stream.modelPath;
                auto [residency, inserted] = m_Residencies.try_emplace(resourceName);
                ++residency->second.references;
                m_TrackedEntities[entity] = resourceName;
                if (inserted)
                    resources.LoadModelAsync(resourceName, stream.modelPath, stream.isStatic);
                stream.isRequested = true;
                stream.state = StreamingState::Loading;
            }

            if (!stream.isResident)
            {
                if (m_FailedResources.contains(resourceName))
                {
                    stream.state = StreamingState::Failed;
                    continue;
                }
                auto model = resources.GetModel(resourceName);
                if (model && model->IsReadyToRender())
                {
                    auto* mesh = scene.TryGetComponent<MeshRendererComponent>(entity);
                    if (!mesh)
                        mesh = &scene.AddComponent<MeshRendererComponent>(entity);
                    mesh->model = std::move(model);
                    mesh->modelName = resourceName;
                    stream.isResident = true;
                    stream.state = StreamingState::Resident;
                    scene.MarkOctreeEntityDirty(entity);
                }
            }
        }
        else if (!canStream || distSq > unloadDistance * unloadDistance)
        {
            if (stream.isRequested)
            {
                LOGGER_INFO("StreamingSystem") << "Unload distance reached, releasing mesh: " << stream.modelPath;
                stream.state = StreamingState::Unloading;
                detachModel(entity, resourceName);
                Release(entity, resources);
                stream.isRequested = false;
                stream.isResident = false;
                stream.state = StreamingState::Unloaded;
            }
        }

        if (canStream && stream.isResident && scene.HasAllComponents<LODComponent, MeshRendererComponent>(entity))
        {
            auto& lod = scene.GetComponent<LODComponent>(entity);
            auto& mesh = scene.GetComponent<MeshRendererComponent>(entity);

            size_t bestLod = 0;
            for (size_t i = 0; i < lod.lodDistancesSq.size(); ++i)
            {
                if (distSq > lod.lodDistancesSq[i])
                {
                    bestLod = i + 1;
                }
                else
                {
                    break;
                }
            }

            if (bestLod < lod.lodModels.size() && lod.lodModels[bestLod])
            {
                if (mesh.model != lod.lodModels[bestLod])
                {
                    mesh.model = lod.lodModels[bestLod];
                    scene.MarkOctreeEntityDirty(entity);
                }
            }
        }
    }
}

std::vector<entt::id_type> StreamingSystem::GetReadComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<WorldTransformComponent>().hash(),
            entt::type_id<LODComponent>().hash()};
}

std::vector<entt::id_type> StreamingSystem::GetWriteComponents() const
{
    return {entt::type_id<StreamingComponent>().hash(), entt::type_id<MeshRendererComponent>().hash()};
}
