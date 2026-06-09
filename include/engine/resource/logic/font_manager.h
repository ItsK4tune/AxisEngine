#pragma once

#include <resource/interface/i_asset_manager.h>
#include <resource/logic/resource_cache.h>
#include <resource/unit/font.h>
#include <memory>
#include <string>

class FontManager : public IAssetManager<Font>
{
public:
    FontManager() = default;
    ~FontManager() = default;

    std::shared_ptr<Font> Load(const std::string& path) override
    {
        return Load(path, path, 16);
    }

    std::shared_ptr<Font> Load(const std::string& name, const std::string& path, unsigned int fontSize);

    std::shared_ptr<Font> Get(const std::string& nameOrPath) override;

    void Unload(const std::string& nameOrPath) override;

    void Clear() override;
    std::vector<std::string> GetAllNames() const override
    {
        return m_Cache.GetAllNames();
    }

private:
    ResourceCache<Font> m_Cache;
};
