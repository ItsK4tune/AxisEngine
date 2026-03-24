#pragma once

#include <render/logic/video_decoder.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>


class VideoManager : public IAssetManager<VideoDecoder> {
public:
    VideoManager() = default;
    ~VideoManager() = default;

    
    std::shared_ptr<VideoDecoder> Load(const std::string& path) override {
        return Load(path, path);
    }

    
    std::shared_ptr<VideoDecoder> Load(const std::string& name, const std::string& path);

    
    std::shared_ptr<VideoDecoder> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Clear() override;

private:
    ResourceCache<VideoDecoder> m_Cache;
};
