#include <resource/logic/model_manager.h>
#include <resource/type/resource_events.h>
#include <core/logic/axis_assert.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <render/interface/i_graphics_context.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <core/logic/job_system.h>
#include <core/logic/filesystem.h>

ModelManager::ModelManager(ModelInstanceManager& instanceManager) 
    : m_InstanceManager(instanceManager) {}

void ModelManager::Initialize() {
    m_ErrorModel = m_Cache.Get("capsuleModel");
    if (!m_ErrorModel) {
        LOGGER_WARN("ModelManager") << "capsuleModel not found in cache during Initialize. Fallback might be limited.";
    }
}

std::shared_ptr<Model> ModelManager::Load(const std::string& name, const std::string& path, bool isStatic, bool async) {
    if (auto existing = m_Cache.Get(name)) {
        return existing;
    }

    std::string fullPath = FileSystem::getPath(path);

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        auto it = m_PathToModelMap.find(fullPath);
        if (it != m_PathToModelMap.end()) {
            auto model = it->second;
            m_Cache.Add(name, model);
            m_NameToPathMap[name] = fullPath;
            m_PathReferenceCounts[fullPath]++;
            m_InstanceManager.RegisterModel(name, model);
            LOGGER_INFO("ModelManager") << "Deduplicated model load for path: " << path << " under name: " << name;
            return model;
        }
    }

    if (async) {
        auto model = std::make_shared<Model>();
        model->SetName(name);
        m_Cache.Add(name, model);

        {
            std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
            m_PathToModelMap[fullPath] = model;
            m_PathReferenceCounts[fullPath] = 1;
            m_NameToPathMap[name] = fullPath;
        }

        auto future = JobSystem::Instance().ExecuteAsync([this, model, path, isStatic, name]() {
            try {
                model->LoadCPU(path, isStatic);
            } catch (...) {
                LOGGER_ERROR("ModelManager") << "Async load failed for: " << path << ". Fallback will be applied on GPU upload.";
            }
        });
        
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        m_PendingModels.push_back({name, model, std::move(future)});
        
        return model;
    } else {
        auto model = std::make_shared<Model>();
        model->SetName(name);
        try {
            model->LoadCPU(path, isStatic);
            if (ServiceLocator::Instance().Has<IGraphicsContext>()) {
                model->UploadToGPU();
            }
            m_Cache.Add(name, model);
            m_InstanceManager.RegisterModel(name, model);

            {
                std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                m_PathToModelMap[fullPath] = model;
                m_PathReferenceCounts[fullPath] = 1;
                m_NameToPathMap[name] = fullPath;
            }

            LOGGER_INFO("ModelManager") << "Loaded model: " << name;
            return model;
        } catch (...) {
            LOGGER_ERROR("ModelManager") << "Failed to load model: " << path << ". Returning cubeModel fallback.";
            if (m_ErrorModel) {
                m_Cache.Add(name, m_ErrorModel);
                return m_ErrorModel;
            }
            return nullptr;
        }
    }
}

std::shared_ptr<Model> ModelManager::Get(const std::string& name) {
    return m_Cache.Get(name);
}

void ModelManager::Unload(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
    auto pathIt = m_NameToPathMap.find(name);
    if (pathIt != m_NameToPathMap.end()) {
        std::string fullPath = pathIt->second;
        m_PathReferenceCounts[fullPath]--;
        if (m_PathReferenceCounts[fullPath] <= 0) {
            m_PathToModelMap.erase(fullPath);
            m_PathReferenceCounts.erase(fullPath);
            m_InstanceManager.UnloadModel(name);
            LOGGER_INFO("ModelManager") << "Fully unloaded physical model: " << fullPath;
        } else {
            m_InstanceManager.UnloadModel(name);
        }
        m_NameToPathMap.erase(pathIt);
    } else {
        m_InstanceManager.UnloadModel(name);
    }
    m_Cache.Remove(name);
}

void ModelManager::Update(float dt) {
    std::lock_guard<std::mutex> lock(m_PendingMutex);

    for (auto it = m_PendingModels.begin(); it != m_PendingModels.end();) {
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            it->future.get();
            
            if (!it->model->IsReadyToRender()) { 
                it->model->UploadToGPU();
                m_InstanceManager.RegisterModel(it->name, it->model);
                LOGGER_INFO("ModelManager") << "Async model finished loading and uploaded to GPU: " << it->name;
                EventManager::Instance().Publish(ResourceLoadedEvent{it->name, "MODEL", true});
            }
            it = m_PendingModels.erase(it);
        } else {
            ++it;
        }
    }
}

void ModelManager::Clear() {
    auto names = m_Cache.GetAllNames();
    for (const auto& name : names) {
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
}
