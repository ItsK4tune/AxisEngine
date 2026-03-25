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


class TextureManager : public IAssetManager<Texture> {
public:
    TextureManager(ITextureManager& lowLevelManager);
    ~TextureManager();

    
    std::shared_ptr<Texture> Load(const std::string& path) override {
        return Load(path, path);
    }

    
    std::shared_ptr<Texture> Load(const std::string& name, const std::string& path, bool async = true, bool keepCpuData = false);

    
    std::shared_ptr<Texture> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Update(float dt = 0.0f) override;

    
    void Clear() override;


    void SetAsyncEnabled(bool enabled) { m_AsyncEnabled = enabled; }
    void SetMaxAnisotropy(float anisotropy) { m_MaxAnisotropy = anisotropy; }

    void Initialize() override;

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
    
    std::shared_ptr<Texture> m_ErrorTexture;
};
