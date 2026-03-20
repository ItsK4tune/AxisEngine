#pragma once

#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>
#include <future>
#include <vector>
#include <mutex>

#include <resource/interface/i_asset_manager.h>

/**
 * @brief High-level manager for Texture assets.
 * Handles synchronous and asynchronous loading using ResourceCache.
 */
class TextureManager : public IAssetManager<Texture> {
public:
    TextureManager(ITextureManager& lowLevelManager);
    ~TextureManager();

    /**
     * @brief Loads a texture from file (IAssetManager implementation).
     */
    std::shared_ptr<Texture> Load(const std::string& path) override {
        return Load(path, path);
    }

    /**
     * @brief Loads a texture from file with custom name.
     */
    std::shared_ptr<Texture> Load(const std::string& name, const std::string& path, bool async = true, bool keepCpuData = false);

    /**
     * @brief Retrieves a texture from cache.
     */
    std::shared_ptr<Texture> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads a texture and deletes its GPU resource.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Processes pending asynchronous loading results.
     */
    void Update(float dt = 0.0f) override;

    /**
     * @brief Clears cache and deletes all GPU textures.
     */
    void Clear() override;

    // Configuration
    void SetAsyncEnabled(bool enabled) { m_AsyncEnabled = enabled; }
    void SetMaxAnisotropy(float anisotropy) { m_MaxAnisotropy = anisotropy; }

private:
    struct TextureData {
        std::string name;
        std::string path;
        int width, height, nrComponents;
        unsigned char* data = nullptr;
        bool keepCpuData = false;
    };

    ITextureManager& m_LowLevelManager;
    ResourceCache<Texture> m_Cache;
    
    std::vector<std::future<TextureData>> m_AsyncLoads;
    std::mutex m_AsyncMutex;
    
    bool m_AsyncEnabled = true;
    float m_MaxAnisotropy = 1.0f;
};
