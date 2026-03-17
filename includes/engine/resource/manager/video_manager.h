#pragma once

#include <render/logic/video_decoder.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Video assets (VideoDecoders).
 */
class VideoManager {
public:
    VideoManager() = default;
    ~VideoManager() = default;

    /**
     * @brief Loads a video file.
     */
    std::shared_ptr<VideoDecoder> Load(const std::string& name, const std::string& path);

    /**
     * @brief Retrieves a video decoder from the cache.
     */
    std::shared_ptr<VideoDecoder> Get(const std::string& name);

    /**
     * @brief Unloads a video from memory.
     */
    void Unload(const std::string& name);

private:
    ResourceCache<VideoDecoder> m_Cache;
};
