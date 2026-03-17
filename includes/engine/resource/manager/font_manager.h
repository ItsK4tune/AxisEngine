#pragma once

#include <render/logic/font.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Font assets.
 */
class FontManager {
public:
    FontManager() = default;
    ~FontManager() = default;

    /**
     * @brief Loads a font file.
     */
    std::shared_ptr<Font> Load(const std::string& name, const std::string& path, unsigned int fontSize);

    /**
     * @brief Retrieves a font from the cache.
     */
    std::shared_ptr<Font> Get(const std::string& name);

    /**
     * @brief Unloads a font from memory.
     */
    void Unload(const std::string& name);

private:
    ResourceCache<Font> m_Cache;
};
