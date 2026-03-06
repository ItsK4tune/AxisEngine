#include <systems/audio/audio_manager.h>
#include <core/utils/logger.h>

AudioManager::AudioManager(std::unique_ptr<IAudioEngine> engine)
    : m_Engine(std::move(engine))
{
}

AudioManager::~AudioManager()
{
    Shutdown();
}

bool AudioManager::Init()
{
    if (!m_Engine)
    {
        LOGGER_ERROR("AudioManager") << "No audio engine provided";
        return false;
    }
    return m_Engine->Init();
}

void AudioManager::Shutdown()
{
    if (m_Engine)
        m_Engine->Shutdown();
}
