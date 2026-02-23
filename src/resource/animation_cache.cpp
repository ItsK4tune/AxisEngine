#include <resource/animation_cache.h>
#include <utils/filesystem.h>
#include <utils/logger.h>

AnimationCache::AnimationCache()
{
}

AnimationCache::~AnimationCache()
{
    Clear();
}

void AnimationCache::LoadAnimation(const std::string& name, const std::string& path, Model* model)
{
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
    if (m_Animations.find(name) != m_Animations.end())
        return m_Animations[name];

    LOGGER_WARN("AnimationCache") << "Animation not found: " << name;
    return nullptr;
}

void AnimationCache::Clear()
{
    m_Animations.clear();
}
