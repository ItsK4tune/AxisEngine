#include <resource/model_instance_manager.h>
#include <iostream>

ModelInstanceManager::ModelInstanceManager()
{
}

ModelInstanceManager::~ModelInstanceManager()
{
    for (auto& pair : m_ModelPools)
    {
        delete pair.second.model;
    }
    m_ModelPools.clear();
}

Model* ModelInstanceManager::GetOrLoadModel(const std::string& name, const std::string& path, bool isStatic)
{
    auto it = m_ModelPools.find(name);
    
    if (it != m_ModelPools.end())
    {
        it->second.refCount++;
        return it->second.model;
    }
    
    Model* model = new Model(path, isStatic);
    
    ModelPool pool;
    pool.model = model;
    pool.refCount = 1;
    
    m_ModelPools[name] = pool;
    
    std::cout << "[ModelInstanceManager] Loaded model '" << name << "': " << path << (isStatic ? " (STATIC)" : " (DYNAMIC)") << std::endl;
    
    return model;
}

void ModelInstanceManager::AddInstance(const std::string& modelPath, const glm::mat4& transform, entt::entity entity)
{
    auto it = m_ModelPools.find(modelPath);
    
    if (it == m_ModelPools.end())
    {
        GetOrLoadModel(modelPath, modelPath, false);
        it = m_ModelPools.find(modelPath);
    }
    
    ModelInstance instance;
    instance.transform = transform;
    instance.entity = entity;
    
    it->second.instances.push_back(instance);
}

void ModelInstanceManager::RemoveInstance(const std::string& modelPath, entt::entity entity)
{
    auto it = m_ModelPools.find(modelPath);
    
    if (it == m_ModelPools.end())
        return;
    
    auto& instances = it->second.instances;
    instances.erase(
        std::remove_if(instances.begin(), instances.end(),
            [entity](const ModelInstance& inst) { return inst.entity == entity; }),
        instances.end()
    );
    
    it->second.refCount--;
}

const std::vector<ModelInstance>& ModelInstanceManager::GetInstances(const std::string& modelPath)
{
    static std::vector<ModelInstance> empty;
    
    auto it = m_ModelPools.find(modelPath);
    if (it == m_ModelPools.end())
        return empty;
    
    return it->second.instances;
}

void ModelInstanceManager::ClearAllInstances()
{
    for (auto& pair : m_ModelPools)
    {
        pair.second.instances.clear();
        pair.second.refCount = 0;
    }
}

void ModelInstanceManager::UnloadUnusedModels()
{
    auto it = m_ModelPools.begin();
    while (it != m_ModelPools.end())
    {
        if (it->second.refCount <= 0 && it->second.instances.empty())
        {
            delete it->second.model;
            std::cout << "[ModelInstanceManager] Unloaded unused model: " << it->first << std::endl;
            it = m_ModelPools.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t ModelInstanceManager::GetInstanceCount(const std::string& modelPath) const
{
    auto it = m_ModelPools.find(modelPath);
    if (it == m_ModelPools.end())
        return 0;
    
    return it->second.instances.size();
}

size_t ModelInstanceManager::GetTotalInstanceCount() const
{
    size_t total = 0;
    for (const auto& pair : m_ModelPools)
    {
        total += pair.second.instances.size();
    }
    return total;
}

size_t ModelInstanceManager::GetLoadedModelCount() const
{
    return m_ModelPools.size();
}
