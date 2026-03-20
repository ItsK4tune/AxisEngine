#pragma once

#include <render/unit/skybox.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>
#include <vector>

/**
 * @brief High-level manager for Skybox assets.
 */
class SkyboxManager : public IAssetManager<Skybox> {
public:
     SkyboxManager() = default;
    ~SkyboxManager() = default;

    /**
     * @brief Loads a skybox from a directory or base path (IAssetManager implementation).
     * @note Convention: path + "/posx.jpg", etc.
     */
    std::shared_ptr<Skybox> Load(const std::string& path) override {
        std::vector<std::string> faces = {
            path + "/right.jpg", path + "/left.jpg",
            path + "/top.jpg",   path + "/bottom.jpg",
            path + "/front.jpg", path + "/back.jpg"
        };
        return Load(path, faces);
    }

    /**
     * @brief Loads a skybox from 6 face textures.
     */
    std::shared_ptr<Skybox> Load(const std::string& name, const std::vector<std::string>& faces);

    /**
     * @brief Retrieves a skybox from the cache.
     */
    std::shared_ptr<Skybox> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads a skybox from memory.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Clears the cache.
     */
    void Clear() override;

private:
    ResourceCache<Skybox> m_Cache;
};
