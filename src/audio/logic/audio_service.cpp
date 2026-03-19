#include <audio/logic/audio_service.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>

AudioService::~AudioService()
{
    Shutdown();
}

bool AudioService::Initialize(std::unique_ptr<IAudioEngine> engine)
{
    if (!engine)
    {
        LOGGER_ERROR("AudioService") << "No audio engine provided";
        return false;
    }

    m_Engine = std::move(engine);
    if (!m_Engine->Initialize())
    {
        LOGGER_WARN("AudioService") << "Audio engine initialization failed, continuing without audio";
        m_Engine.reset();
        return false;
    }

    ServiceLocator::Instance().Register<AudioService>(this);
    LOGGER_INFO("AudioService") << "Audio service initialized successfully.";
    return true;
}

void AudioService::Shutdown()
{
    ServiceLocator::Instance().Unregister<AudioService>();
    if (m_Engine)
    {
        m_Engine->Shutdown();
        m_Engine.reset();
    }
}

std::shared_ptr<ISound> AudioService::Play2D(const std::string& path, bool loop)
{
    if (!m_Engine) return nullptr;
    return m_Engine->Play2D(path, loop);
}

std::shared_ptr<ISound> AudioService::Play3D(const std::string& path, glm::vec3 pos, bool loop)
{
    if (!m_Engine) return nullptr;
    return m_Engine->Play3D(path, pos, loop);
}

std::shared_ptr<ISound> AudioService::Play2D(std::shared_ptr<IAudioSource> source, bool loop)
{
    if (!m_Engine || !source) return nullptr;
    return m_Engine->Play2D(source.get(), loop);
}

void AudioService::UpdateListener(glm::vec3 position, glm::vec3 lookDir)
{
    if (m_Engine)
        m_Engine->SetListenerPosition(position, lookDir);
}

void AudioService::SetGlobalVolume(float volume)
{
    if (m_Engine)
        m_Engine->SetGlobalVolume(volume);
}

void AudioService::StopAll()
{
    if (m_Engine)
        m_Engine->StopAllSounds();
}
