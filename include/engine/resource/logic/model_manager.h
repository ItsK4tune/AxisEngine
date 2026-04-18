#pragma once

#include <resource/unit/model.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <resource/logic/model_instance_manager.h>
#include <string>
#include <memory>
#include <future>
#include <vector>


class ModelManager : public IAssetManager<Model> {
public:
    ModelManager(ModelInstanceManager& instanceManager);
    ~ModelManager() = default;

    
    std::shared_ptr<Model> Load(const std::string& path) override {
        return Load(path, path);
    }

    
    std::shared_ptr<Model> Load(const std::string& name, const std::string& path, bool isStatic = false, bool async = false);

    
    std::shared_ptr<Model> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Update(float dt = 0.0f) override;

    
    void Clear() override;

    std::vector<std::string> GetAllNames() const override {
        return m_Cache.GetAllNames();
    }

    ModelInstanceManager& GetInstanceManager() { return m_InstanceManager; }

    void Initialize() override;

private:
    ModelInstanceManager& m_InstanceManager;
    ResourceCache<Model> m_Cache;
    std::shared_ptr<Model> m_ErrorModel;

    struct PendingModel {
        std::string name;
        std::shared_ptr<Model> model;
        std::future<void> future;
    };
    std::vector<PendingModel> m_PendingModels;
    std::mutex m_PendingMutex;
};
