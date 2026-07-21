#include <resource/logic/model_manager.h>
#include <core/logic/axis_assert.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <render/interface/i_graphics_context.h>
#include <resource/type/resource_events.h>

void ModelManager::Initialize()
{
    m_ErrorModel = m_Cache.Get("capsuleModel");
    if (!m_ErrorModel)
    {
        LOGGER_WARN("ModelManager") << "capsuleModel not found in cache during Initialize. Fallback might be limited.";
    }
}

std::shared_ptr<Model> ModelManager::Load(const std::string& name, const std::string& path, bool isStatic, bool async)
{
    if (auto existing = m_Cache.Get(name))
    {
        return existing;
    }

    std::string fullPath = FileSystem::getPath(path);

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        auto it = m_PathToModelMap.find(fullPath);
        if (it != m_PathToModelMap.end())
        {
            auto model = it->second;
            m_Cache.Add(name, model);
            m_NameToPathMap[name] = fullPath;
            m_PathReferenceCounts[fullPath]++;
            LOGGER_INFO("ModelManager") << "Deduplicated model load for path: " << path << " under name: " << name;
            return model;
        }
    }

    if (async)
    {
        auto model = std::make_shared<Model>();
        model->SetName(name);
        auto decodedModel = std::make_shared<Model>();
        decodedModel->SetName(name);
        m_Cache.Add(name, model);

        {
            std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
            m_PathToModelMap[fullPath] = model;
            m_PathReferenceCounts[fullPath] = 1;
            m_NameToPathMap[name] = fullPath;
        }

        auto future = JobSystem::Instance().ExecuteAsync([decodedModel, path, isStatic]() {
            try
            {
                decodedModel->LoadCPU(path, isStatic);
                return !decodedModel->meshes.empty();
            }
            catch (...)
            {
                LOGGER_ERROR("ModelManager") << "Async load failed for: " << path;
                return false;
            }
        });

        std::lock_guard<std::mutex> lock(m_PendingMutex);
        const uint64_t generation = ++m_LoadGenerations[fullPath];
        m_PendingModels.push_back({name, fullPath, model, decodedModel, std::move(future), generation});

        return model;
    }
    else
    {
        auto model = std::make_shared<Model>();
        model->SetName(name);
        try
        {
            model->LoadCPU(path, isStatic);
            if (model->meshes.empty())
            {
                LOGGER_ERROR("ModelManager") << "Loaded model has no meshes: " << path;
                if (m_StrictLoading)
                    return nullptr;
                if (m_ErrorModel)
                {
                    LOGGER_WARN("ModelManager") << "Returning fallback model for: " << path;
                    m_Cache.Add(name, m_ErrorModel);
                    return m_ErrorModel;
                }
                return nullptr;
            }
            if (ServiceLocator::Instance().Has<IGraphicsContext>())
            {
                model->UploadToGPU();
                if (m_DiscardCpuMeshDataAfterUpload)
                    model->ReleaseCpuMeshData();
            }
            m_Cache.Add(name, model);
            {
                std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                m_PathToModelMap[fullPath] = model;
                m_PathReferenceCounts[fullPath] = 1;
                m_NameToPathMap[name] = fullPath;
            }

            LOGGER_INFO("ModelManager") << "Loaded model: " << name;
            return model;
        }
        catch (...)
        {
            LOGGER_ERROR("ModelManager") << "Failed to load model: " << path << "."
                                         << (m_StrictLoading ? " Strict loading is enabled."
                                                             : " Returning fallback model.");
            if (m_StrictLoading)
                return nullptr;
            if (m_ErrorModel)
            {
                m_Cache.Add(name, m_ErrorModel);
                return m_ErrorModel;
            }
            return nullptr;
        }
    }
}

std::shared_ptr<Model> ModelManager::Get(const std::string& name)
{
    return m_Cache.Get(name);
}

void ModelManager::Unload(const std::string& name)
{
    std::string fullyUnloadedPath;
    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        auto pathIt = m_NameToPathMap.find(name);
        if (pathIt != m_NameToPathMap.end())
        {
            std::string fullPath = pathIt->second;
            m_PathReferenceCounts[fullPath]--;
            if (m_PathReferenceCounts[fullPath] <= 0)
            {
                m_PathToModelMap.erase(fullPath);
                m_PathReferenceCounts.erase(fullPath);
                fullyUnloadedPath = fullPath;
                LOGGER_INFO("ModelManager") << "Fully unloaded physical model: " << fullPath;
            }
            m_NameToPathMap.erase(pathIt);
        }
    }
    if (!fullyUnloadedPath.empty())
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        ++m_LoadGenerations[fullyUnloadedPath];
    }
    m_Cache.Remove(name);
}

void ModelManager::Update(float dt)
{
    std::vector<PendingModel> completed;
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        const size_t completionLimit =
            m_CompletedLoadBudgetEnabled ? m_MaxCompletedLoadsPerFrame : m_PendingModels.size();
        completed.reserve((std::min)(completionLimit, m_PendingModels.size()));
        for (auto it = m_PendingModels.begin();
             it != m_PendingModels.end() && completed.size() < completionLimit;)
        {
            if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                completed.push_back(std::move(*it));
                it = m_PendingModels.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto& pending : completed)
    {
        const bool loaded = pending.future.get();
        bool currentGeneration = false;
        {
            std::lock_guard<std::mutex> lock(m_PendingMutex);
            const auto generation = m_LoadGenerations.find(pending.path);
            currentGeneration = generation != m_LoadGenerations.end() && generation->second == pending.generation;
        }
        if (!currentGeneration)
            continue;

        if (!loaded)
        {
            LOGGER_ERROR("ModelManager")
                << "Async model failed to load: " << pending.name
                << (m_StrictLoading ? ". Strict loading is enabled." : ". Applying fallback model.");
            {
                std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                m_PathToModelMap.erase(pending.path);
                m_PathReferenceCounts.erase(pending.path);
            }
            std::vector<std::string> aliases;
            {
                std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                for (auto it = m_NameToPathMap.begin(); it != m_NameToPathMap.end();)
                {
                    if (it->second == pending.path)
                    {
                        aliases.push_back(it->first);
                        it = m_NameToPathMap.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            for (const auto& alias : aliases)
            {
                if (!m_StrictLoading && m_ErrorModel)
                    m_Cache.Add(alias, m_ErrorModel);
                else
                    m_Cache.Remove(alias);
                EventManager::Instance().Publish(ResourceLoadedEvent{alias, "MODEL", false});
            }
            continue;
        }

        pending.model->AdoptCpuData(std::move(*pending.decodedModel));

        if (!pending.model->IsReadyToRender() && ServiceLocator::Instance().Has<IGraphicsContext>())
        {
            pending.model->UploadToGPU();
            if (m_DiscardCpuMeshDataAfterUpload)
                pending.model->ReleaseCpuMeshData();
            LOGGER_INFO("ModelManager") << "Async model finished loading and uploaded to GPU: " << pending.name;
        }
        std::vector<std::string> aliases;
        {
            std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
            for (const auto& [alias, path] : m_NameToPathMap)
                if (path == pending.path)
                    aliases.push_back(alias);
        }
        for (const auto& alias : aliases)
            EventManager::Instance().Publish(ResourceLoadedEvent{alias, "MODEL", true});
    }
}

void ModelManager::Clear()
{
    auto names = m_Cache.GetAllNames();
    for (const auto& name : names)
    {
        Unload(name);
    }
    m_Cache.Clear();

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        m_PathToModelMap.clear();
        m_PathReferenceCounts.clear();
        m_NameToPathMap.clear();
    }

    std::lock_guard<std::mutex> lock(m_PendingMutex);
    m_PendingModels.clear();
    m_LoadGenerations.clear();
}
