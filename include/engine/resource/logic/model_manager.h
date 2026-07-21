#pragma once

#include <resource/interface/i_asset_manager.h>
#include <resource/logic/resource_cache.h>
#include <resource/unit/model.h>
#include <future>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

class ModelManager : public IAssetManager<Model>
{
public:
    ModelManager() = default;
    ~ModelManager() = default;

    std::shared_ptr<Model> Load(const std::string& path) override
    {
        return Load(path, path);
    }

    std::shared_ptr<Model> Load(const std::string& name, const std::string& path, bool isStatic = false,
                                bool async = false);

    std::shared_ptr<Model> Get(const std::string& nameOrPath) override;

    void Unload(const std::string& nameOrPath) override;

    void Update(float dt = 0.0f) override;

    void Clear() override;

    std::vector<std::string> GetAllNames() const override
    {
        return m_Cache.GetAllNames();
    }

    void SetStrictLoading(bool strict)
    {
        m_StrictLoading = strict;
    }
    void SetCompletedLoadBudget(bool enabled, size_t maxPerFrame)
    {
        m_CompletedLoadBudgetEnabled = enabled;
        m_MaxCompletedLoadsPerFrame = (std::max)(size_t{1}, maxPerFrame);
    }
    void SetDiscardCpuMeshDataAfterUpload(bool enabled)
    {
        if (enabled && !m_DiscardCpuMeshDataAfterUpload)
        {
            for (const auto& name : m_Cache.GetAllNames())
            {
                if (auto model = m_Cache.Get(name); model && model->IsReadyToRender())
                    model->ReleaseCpuMeshData();
            }
        }
        m_DiscardCpuMeshDataAfterUpload = enabled;
    }

    void Initialize() override;

private:
    std::unordered_map<std::string, std::shared_ptr<Model>> m_PathToModelMap;
    std::unordered_map<std::string, int> m_PathReferenceCounts;
    std::unordered_map<std::string, std::string> m_NameToPathMap;
    std::mutex m_DeduplicationMutex;

    ResourceCache<Model> m_Cache;
    std::shared_ptr<Model> m_ErrorModel;

    struct PendingModel
    {
        std::string name;
        std::string path;
        std::shared_ptr<Model> model;
        std::shared_ptr<Model> decodedModel;
        std::future<bool> future;
        uint64_t generation = 0;
    };
    std::vector<PendingModel> m_PendingModels;
    std::unordered_map<std::string, uint64_t> m_LoadGenerations;
    std::mutex m_PendingMutex;
    size_t m_MaxCompletedLoadsPerFrame = 2;
    bool m_CompletedLoadBudgetEnabled = true;
    bool m_DiscardCpuMeshDataAfterUpload = false;
    bool m_StrictLoading = false;
};
