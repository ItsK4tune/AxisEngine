#pragma once

#include <entt/entt.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <graphics/geometry/model.h>
#include <memory>
#include <mutex>
#include <resource/model_types.h>
#include <string>
#include <unordered_map>
#include <vector>

class ModelInstanceManager
{
public:
    ModelInstanceManager() = default;
    ~ModelInstanceManager() = default;

    std::shared_ptr<Model> GetOrLoadModel(const std::string& name, const std::string& path, bool isStatic = false);
    void RegisterModel(const std::string& name, std::shared_ptr<Model> model);
    void AddInstance(const std::string& modelPath, const glm::mat4& transform, entt::entity entity);
    void RemoveInstance(const std::string& modelPath, entt::entity entity);

    const std::vector<ModelInstance>& GetInstances(const std::string& modelPath);

    void ClearAllInstances();
    void Clear() { ClearAllInstances(); }
    void UnloadUnusedModels();
    bool UnloadModel(const std::string& name);

    size_t GetInstanceCount(const std::string& modelPath) const;
    size_t GetTotalInstanceCount() const;
    size_t GetLoadedModelCount() const;

private:
    struct ModelPool
    {
        std::shared_ptr<Model> model;
        std::vector<ModelInstance> instances;
    };

    std::unordered_map<std::string, ModelPool> m_ModelPools;
    mutable std::recursive_mutex m_PoolMutex;
};
