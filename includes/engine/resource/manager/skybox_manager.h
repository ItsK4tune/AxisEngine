#pragma once

#include <render/logic/skybox.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>
#include <vector>

/**
 * @brief High-level manager for Skybox assets.
 */
class SkyboxManager {
public:
     SkyboxManager() = default;
    ~SkyboxManager() = default;

    /**
     * @brief Loads a skybox from 6 face textures.
     */
    std::shared_ptr<Skybox> Load(const std::string& name, const std::vector<std::string>& faces);

    /**
     * @brief Retrieves a skybox from the cache.
     */
    std::shared_ptr<Skybox> Get(const std::string& name);

    /**
     * @brief Unloads a skybox from memory.
     */
    void Unload(const std::string& name);

private:
    ResourceCache<Skybox> m_Cache;
};
