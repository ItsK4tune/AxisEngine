#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <render/logic/animation.h>
#include <string>

class AnimationCache
{
public:
    AnimationCache();
    ~AnimationCache();

    void LoadAnimation(const std::string& name, const std::string& path, Model* model);
    std::shared_ptr<Animation> GetAnimation(const std::string& name);
    void Remove(const std::string& name);
    void Clear();

private:
    std::map<std::string, std::shared_ptr<Animation>> m_Animations;
    mutable std::mutex m_Mutex;
};