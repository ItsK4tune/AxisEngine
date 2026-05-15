#pragma once

#include <audio/interface/i_audio_engine.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>


class AudioAssetManager : public IAssetManager<IAudioSource> {
public:
    AudioAssetManager(IAudioEngine& audioEngine);
    ~AudioAssetManager() = default;

    
    std::shared_ptr<IAudioSource> Load(const std::string& path) override {
        return Load(path, path);
    }

    
    std::shared_ptr<IAudioSource> Load(const std::string& name, const std::string& path);

    
    std::shared_ptr<IAudioSource> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Clear() override;
    
    std::vector<std::string> GetAllNames() const { return m_Cache.GetAllNames(); }

private:
    IAudioEngine& m_AudioEngine;
    ResourceCache<IAudioSource> m_Cache;
};
