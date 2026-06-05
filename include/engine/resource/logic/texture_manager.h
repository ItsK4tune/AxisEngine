#pragma once

#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_asset_manager.h>
#include <resource/logic/resource_cache.h>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TextureManager : public IAssetManager<Texture>
{
public:
    TextureManager(ITextureManager& lowLevelManager);
    ~TextureManager();

    std::shared_ptr<Texture> Load(const std::string& path) override
    {
        return Load(path, path);
    }

    std::shared_ptr<Texture> Load(const std::string& name, const std::string& path, bool async = true,
                                  bool keepCpuData = false);

    std::shared_ptr<Texture> CreateFromData(const std::string& name, const unsigned char* pixels, int width, int height, int nrComponents, bool keepCpuData = false);

    std::shared_ptr<Texture> Get(const std::string& nameOrPath) override;

    void Unload(const std::string& nameOrPath) override;
    bool Reload(const std::string& name);

    void Update(float dt = 0.0f) override;

    void Clear() override;

    std::vector<std::string> GetAllNames() const override
    {
        return m_Cache.GetAllNames();
    }

    void SetAsyncEnabled(bool enabled)
    {
        m_AsyncEnabled = enabled;
    }
    void SetMaxAnisotropy(float anisotropy)
    {
        m_MaxAnisotropy = anisotropy;
    }
    void SetStrictLoading(bool strict)
    {
        m_StrictLoading = strict;
    }

    void Initialize() override;

private:
    struct TextureData
    {
        std::string name;
        std::string path;
        int width, height, nrComponents;
        unsigned char* data = nullptr;
        bool keepCpuData = false;
    };

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_PathToTextureMap;
    std::unordered_map<std::string, int> m_PathReferenceCounts;
    std::unordered_map<std::string, std::string> m_NameToPathMap;
    std::unordered_map<std::string, bool> m_NameKeepCpuDataMap;
    std::mutex m_DeduplicationMutex;

    ITextureManager& m_LowLevelManager;
    ResourceCache<Texture> m_Cache;

    std::vector<std::future<TextureData>> m_AsyncLoads;
    std::mutex m_AsyncMutex;

    bool m_AsyncEnabled = true;
    float m_MaxAnisotropy = 1.0f;
    bool m_StrictLoading = false;

    std::shared_ptr<Texture> m_ErrorTexture;
};
