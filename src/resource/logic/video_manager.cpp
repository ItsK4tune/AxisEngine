#include <resource/logic/video_manager.h>
#include <core/logic/logger.h>

std::shared_ptr<VideoDecoder> VideoManager::Load(const std::string& name, const std::string& path)
{
    if (auto existing = m_Cache.Get(name))
        return existing;

    auto decoder = std::make_shared<VideoDecoder>();
    if (decoder->Load(path))
    {
        m_Cache.Add(name, decoder);
        LOGGER_INFO("VideoManager") << "Loaded video: " << name;
        return decoder;
    }

    LOGGER_ERROR("VideoManager") << "Failed to load video: " << path;
    return nullptr;
}

std::shared_ptr<VideoDecoder> VideoManager::Get(const std::string& nameOrPath)
{
    return m_Cache.Get(nameOrPath);
}

void VideoManager::Unload(const std::string& nameOrPath)
{
    m_Cache.Remove(nameOrPath);
}

void VideoManager::Clear()
{
    m_Cache.Clear();
}
