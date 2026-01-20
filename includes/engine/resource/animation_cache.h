#pragma once

#include <graphic/animation.h>
#include <string>
#include <map>
#include <memory>

class AnimationCache
{
public:
    AnimationCache();
    ~AnimationCache();

    void LoadAnimation(const std::string& name, const std::string& path, Model* model);
    Animation* GetAnimation(const std::string& name);
    
    void Clear();

private:
    std::map<std::string, std::unique_ptr<Animation>> m_Animations;
};
