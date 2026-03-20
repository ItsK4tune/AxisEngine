#pragma once

#include <render/logic/video_decoder.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Video assets (VideoDecoders).
 */
class VideoManager : public IAssetManager<VideoDecoder> {
public:
    VideoManager() = default;
    ~VideoManager() = default;

    /**
     * @brief Loads a video file (IAssetManager implementation).
     */
    std::shared_ptr<VideoDecoder> Load(const std::string& path) override {
        return Load(path, path);
    }

    /**
     * @brief Loads a video file.
     */
    std::shared_ptr<VideoDecoder> Load(const std::string& name, const std::string& path);

    /**
     * @brief Retrieves a video decoder from the cache.
     */
    std::shared_ptr<VideoDecoder> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads a video from memory.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Clears the cache.
     */
    void Clear() override;

private:
    ResourceCache<VideoDecoder> m_Cache;
};
