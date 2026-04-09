#include <resource/logic/model_manager.h>
#include <resource/type/resource_events.h>
#include <core/logic/axis_assert.h>
#include <core/logic/logger.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <core/logic/job_system.h>

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

    if (async) {
        auto model = std::make_shared<Model>();
        m_Cache.Add(name, model);

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
        try {
            model->LoadCPU(path, isStatic);
            model->UploadToGPU();
            m_Cache.Add(name, model);
            m_InstanceManager.RegisterModel(name, model);
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
    m_Cache.Remove(name);
    m_InstanceManager.UnloadModel(name);
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
    m_Cache.Clear();
    std::lock_guard<std::mutex> lock(m_PendingMutex);
    m_PendingModels.clear();
}
