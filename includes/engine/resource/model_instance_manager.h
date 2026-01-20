#pragma once

#include <entt/entt.hpp>
#include <graphic/model.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

struct ModelInstance
{
    glm::mat4 transform;
    entt::entity entity;
};

class ModelInstanceManager
{
public:
    ModelInstanceManager();
    ~ModelInstanceManager();

    Model* GetOrLoadModel(const std::string& name, const std::string& path, bool isStatic = false);
    void AddInstance(const std::string& modelPath, const glm::mat4& transform, entt::entity entity);
    void RemoveInstance(const std::string& modelPath, entt::entity entity);
    
    const std::vector<ModelInstance>& GetInstances(const std::string& modelPath);
    
    void ClearAllInstances();
    void Clear() { ClearAllInstances(); }
    void UnloadUnusedModels();
    
    size_t GetInstanceCount(const std::string& modelPath) const;
    size_t GetTotalInstanceCount() const;
    size_t GetLoadedModelCount() const;

private:
    struct ModelPool
    {
        Model* model;
        std::vector<ModelInstance> instances;
        int refCount;
    };

    std::unordered_map<std::string, ModelPool> m_ModelPools;
};
