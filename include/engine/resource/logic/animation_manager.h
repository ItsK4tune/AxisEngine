#pragma once

#include <resource/unit/animation.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Animation assets.
 */
class ModelManager;

/**
 * @brief High-level manager for Animation assets.
 */
class AnimationManager : public IAssetManager<Animation> {
public:
    AnimationManager(ModelManager& modelManager);
    ~AnimationManager() = default;

    /**
     * @brief Loads an animation from file (IAssetManager implementation).
     * @note This requires the model to be loaded first or name-consistent.
     */
    std::shared_ptr<Animation> Load(const std::string& path) override {
        return Load(path, path, ""); // Needs logic to find model name if possible
    }

    /**
     * @brief Loads an animation from file with explicit model.
     */
    std::shared_ptr<Animation> Load(const std::string& name, const std::string& path, const std::string& modelName);

    /**
     * @brief Retrieves an animation from the cache.
     */
    std::shared_ptr<Animation> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads an animation from memory.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Clears the cache.
     */
    void Clear() override;

private:
    ModelManager& m_ModelManager;
    ResourceCache<Animation> m_Cache;
};
