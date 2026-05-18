#include <resource/logic/animation_manager.h>
#include <core/logic/logger.h>
#include <resource/logic/model_manager.h>

AnimationManager::AnimationManager(ModelManager& modelManager) : m_ModelManager(modelManager)
{
}

std::shared_ptr<Animation> AnimationManager::Load(const std::string& name, const std::string& path,
                                                  const std::string& modelName)
{
    if (auto existing = m_Cache.Get(name))
        return existing;

    auto model = m_ModelManager.Get(modelName);
    if (!model)
    {
        LOGGER_ERROR("AnimationManager") << "Cannot load animation '" << name << "': Model '" << modelName
                                         << "' not found.";
        return nullptr;
    }

    auto animation = std::make_shared<Animation>(path, *model);
    if (animation)
    {
        m_Cache.Add(name, animation);
        LOGGER_INFO("AnimationManager") << "Loaded animation: " << name;
        return animation;
    }

    LOGGER_ERROR("AnimationManager") << "Failed to load animation: " << path;
    return nullptr;
}

std::shared_ptr<Animation> AnimationManager::Get(const std::string& nameOrPath)
{
    return m_Cache.Get(nameOrPath);
}

void AnimationManager::Unload(const std::string& nameOrPath)
{
    m_Cache.Remove(nameOrPath);
}

void AnimationManager::Clear()
{
    m_Cache.Clear();
}
