#include <resource/model_instance_manager.h>
#include <resource/model_instance_manager.h>
#include <utils/logger.h>

std::shared_ptr<Model> ModelInstanceManager::GetOrLoadModel(const std::string& name, const std::string& path, bool isStatic)
{
    auto it = m_ModelPools.find(name);

    if (it != m_ModelPools.end())
    {
        return it->second.model;
    }

    std::shared_ptr<Model> model = std::make_shared<Model>(path, isStatic);

    ModelPool pool;
    pool.model = model;

    m_ModelPools[name] = pool;

    LOGGER_DEBUG("ModelInstanceManager") << "Loaded model '" << name << "': " << path << (isStatic ? " (STATIC)" : " (DYNAMIC)");

    return model;
}

void ModelInstanceManager::RegisterModel(const std::string& name, std::shared_ptr<Model> model)
{
    if (m_ModelPools.find(name) != m_ModelPools.end())
        return;

    ModelPool pool;
    pool.model = std::move(model);
    m_ModelPools[name] = std::move(pool);

    LOGGER_DEBUG("ModelInstanceManager") << "Registered async model '" << name << "'";
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
    }
}

void ModelInstanceManager::UnloadUnusedModels()
{
    auto it = m_ModelPools.begin();
    while (it != m_ModelPools.end())
    {
        if (it->second.model.use_count() == 1 && it->second.instances.empty())
        {
            LOGGER_INFO("ModelInstanceManager") << "Unloaded unused model: " << it->first;
            it = m_ModelPools.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool ModelInstanceManager::UnloadModel(const std::string& name)
{
    auto it = m_ModelPools.find(name);
    if (it != m_ModelPools.end())
    {
        if (it->second.model.use_count() == 1 && it->second.instances.empty())
        {
            m_ModelPools.erase(it);
            LOGGER_INFO("ModelInstanceManager") << "Unloaded model: " << name;
            return true;
        }
        else
        {
            LOGGER_WARN("ModelInstanceManager") << "Cannot unload model '" << name << "': still in use (UseCount: " << it->second.model.use_count() << ", Instances: " << it->second.instances.size() << ")";
            return false;
        }
    }
    return false;
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
