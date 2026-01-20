#include <resource/animation_cache.h>
#include <utils/filesystem.h>
#include <iostream>

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
        std::cerr << "[AnimationCache] Model is null for animation: " << name << std::endl;
        return;
    }
    
    m_Animations[name] = std::make_unique<Animation>(FileSystem::getPath(path), model);
    std::cout << "[AnimationCache] Loaded animation: " << name << std::endl;
}

Animation* AnimationCache::GetAnimation(const std::string& name)
{
    if (m_Animations.find(name) != m_Animations.end())
        return m_Animations[name].get();
    
    std::cerr << "[AnimationCache] Animation not found: " << name << std::endl;
    return nullptr;
}

void AnimationCache::Clear()
{
    m_Animations.clear();
}
