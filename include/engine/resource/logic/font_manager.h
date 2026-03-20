#pragma once

#include <render/unit/font.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Font assets.
 */
class FontManager : public IAssetManager<Font> {
public:
    FontManager() = default;
    ~FontManager() = default;

    /**
     * @brief Loads a font file (IAssetManager implementation).
     */
    std::shared_ptr<Font> Load(const std::string& path) override {
        return Load(path, path, 16);
    }

    /**
     * @brief Loads a font file with explicit size.
     */
    std::shared_ptr<Font> Load(const std::string& name, const std::string& path, unsigned int fontSize);

    /**
     * @brief Retrieves a font from the cache.
     */
    std::shared_ptr<Font> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads a font from memory.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Clears all cached fonts.
     */
    void Clear() override;

private:
    ResourceCache<Font> m_Cache;
};
