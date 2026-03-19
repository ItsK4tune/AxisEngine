#pragma once

#include <render/unit/animation.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Animation assets.
 */
class ModelManager;

/**
 * @brief High-level manager for Animation assets.
 */
class AnimationManager {
public:
    AnimationManager(ModelManager& modelManager);
    ~AnimationManager() = default;

    /**
     * @brief Loads an animation from file.
     */
    std::shared_ptr<Animation> Load(const std::string& name, const std::string& path, const std::string& modelName);

    /**
     * @brief Retrieves an animation from the cache.
     */
    std::shared_ptr<Animation> Get(const std::string& name);

    /**
     * @brief Unloads an animation from memory.
     */
    void Unload(const std::string& name);

private:
    ModelManager& m_ModelManager;
    ResourceCache<Animation> m_Cache;
};
