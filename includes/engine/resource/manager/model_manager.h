#pragma once

#include <render/logic/model.h>
#include <resource/logic/resource_cache.h>
#include <resource/manager/model_instance_manager.h>
#include <string>
#include <memory>
#include <future>
#include <vector>

/**
 * @brief High-level manager for Model assets.
 * Coordinates with ModelInstanceManager for scene instances and utilizes ResourceCache for storage.
 */
class ModelManager {
public:
    ModelManager(ModelInstanceManager& instanceManager);
    ~ModelManager() = default;

    /**
     * @brief Loads a model from file.
     */
    std::shared_ptr<Model> Load(const std::string& name, const std::string& path, bool isStatic = false, bool async = false);

    /**
     * @brief Retrieves a model from the cache.
     */
    std::shared_ptr<Model> Get(const std::string& name);

    /**
     * @brief Unloads a model from memory.
     */
    void Unload(const std::string& name);

    /**
     * @brief Processes pending asynchronous model loads.
     */
    void Update();

    /**
     * @brief Clears the cache.
     */
    void Clear();

    ModelInstanceManager& GetInstanceManager() { return m_InstanceManager; }

private:
    ModelInstanceManager& m_InstanceManager;
    ResourceCache<Model> m_Cache;

    struct PendingModel {
        std::string name;
        std::shared_ptr<Model> model;
    };
    std::vector<PendingModel> m_PendingModels;
    std::vector<std::future<void>> m_ActiveFutures;
    std::mutex m_PendingMutex;
};
