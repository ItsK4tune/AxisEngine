#include <resource/logic/animation_cache.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>

AnimationCache::AnimationCache()
{
}

AnimationCache::~AnimationCache()
{
    Clear();
}

void AnimationCache::LoadAnimation(const std::string& name, const std::string& path, Model* model)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!model)
    {
        LOGGER_ERROR("AnimationCache") << "Model is null for animation: " << name;
        return;
    }

    m_Animations[name] = std::make_shared<Animation>(FileSystem::getPath(path), *model);
    LOGGER_INFO("AnimationCache") << "Loaded animation: " << name;
}

std::shared_ptr<Animation> AnimationCache::GetAnimation(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Animations.find(name) != m_Animations.end())
        return m_Animations[name];

    LOGGER_WARN("AnimationCache") << "Animation not found: " << name;
    return nullptr;
}

void AnimationCache::Remove(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Animations.erase(name);
}

void AnimationCache::Clear()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Animations.clear();
}
