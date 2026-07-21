#include <audio/logic/audio_service.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <algorithm>
#include <cmath>

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
    ServiceLocator::Instance().Register<IAudioEngine>(m_Engine.get());
    LOGGER_INFO("AudioService") << "Audio service initialized successfully.";
    return true;
}

void AudioService::Shutdown()
{
    ServiceLocator::Instance().Unregister<AudioService>();
    ServiceLocator::Instance().Unregister<IAudioEngine>();
    if (m_Engine)
    {
        m_Engine->Shutdown();
        m_Engine.reset();
    }
    m_Pulses.clear();
}

std::shared_ptr<ISound> AudioService::Play2D(const std::string& path, bool loop)
{
    if (!m_Engine)
        return nullptr;
    return m_Engine->Play2D(path, loop);
}

std::shared_ptr<ISound> AudioService::Play3D(const std::string& path, glm::vec3 pos, bool loop)
{
    if (!m_Engine)
        return nullptr;
    return m_Engine->Play3D(path, pos, loop);
}

std::shared_ptr<ISound> AudioService::Play2D(std::shared_ptr<IAudioSource> source, bool loop)
{
    if (!m_Engine || !source)
        return nullptr;
    return m_Engine->Play2D(source.get(), loop);
}

void AudioService::UpdateListener(glm::vec3 position, glm::vec3 lookDir)
{
    if (m_Engine)
        m_Engine->SetListenerPosition(position, lookDir);
}

void AudioService::EmitPulse(const glm::vec3& origin, float intensity, float duration, float peak)
{
    if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) || !std::isfinite(intensity) ||
        !std::isfinite(duration) || duration <= 0.0f)
        return;

    AudioPulse pulse;
    pulse.origin = origin;
    pulse.intensity = std::clamp(intensity, 0.0f, 1.0f);
    pulse.peak = !std::isfinite(peak) || peak < 0.0f ? pulse.intensity : std::clamp(peak, 0.0f, 1.0f);
    pulse.duration = duration;
    m_Pulses.push_back(pulse);

    if (m_Pulses.size() > AudioPulseLimits::MaxPulses)
    {
        m_Pulses.erase(m_Pulses.begin(),
                       m_Pulses.begin() + static_cast<std::ptrdiff_t>(m_Pulses.size() - AudioPulseLimits::MaxPulses));
    }
}

void AudioService::UpdatePulses(float deltaTime)
{
    const float dt = std::isfinite(deltaTime) ? std::max(deltaTime, 0.0f) : 0.0f;
    for (auto& pulse : m_Pulses) pulse.age += dt;
    std::erase_if(m_Pulses, [](const AudioPulse& pulse) { return pulse.age >= pulse.duration; });
}

void AudioService::ClearPulses()
{
    m_Pulses.clear();
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
