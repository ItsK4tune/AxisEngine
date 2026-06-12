#include <audio/strategy/null/null_audio_engine.h>
#include <audio/interface/i_audio_listener.h>
#include <audio/interface/i_audio_event.h>
#include <audio/interface/i_audio_stream.h>
#include <core/logic/backend_factory_registry.h>
#include <algorithm>
#include <memory>
#include <utility>

namespace
{
constexpr float kMaxPublicVolume = 100.0f;

float ClampPublicVolume(float volume)
{
    return std::clamp(volume, 0.0f, kMaxPublicVolume);
}

float ApplyGlobalVolume(float volume, float globalVolume)
{
    return ClampPublicVolume(volume) * ClampPublicVolume(globalVolume) / kMaxPublicVolume;
}
}  // namespace

class NullAudioListener final : public IAudioListener
{
public:
    void SetPosition(const glm::vec3& pos) override { m_Position = pos; }
    glm::vec3 GetPosition() const override { return m_Position; }
    void SetOrientation(const glm::vec3& forward, const glm::vec3& up) override
    {
        m_Forward = forward;
        m_Up = up;
    }
    glm::vec3 GetForward() const override { return m_Forward; }
    glm::vec3 GetUp() const override { return m_Up; }
    void SetVelocity(const glm::vec3& vel) override { m_Velocity = vel; }
    glm::vec3 GetVelocity() const override { return m_Velocity; }

private:
    glm::vec3 m_Position{0.0f};
    glm::vec3 m_Forward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Velocity{0.0f};
};

class NullAudioEvent final : public IAudioEvent
{
public:
    NullAudioEvent(std::string name) : m_Name(std::move(name)) {}
    ~NullAudioEvent() override = default;

    std::string GetName() const override { return m_Name; }
    void Play() override { m_Playing = true; }
    void Stop() override { m_Playing = false; }
    void Pause() override { m_Playing = false; }
    void Resume() override { m_Playing = true; }
    bool IsPlaying() const override { return m_Playing; }
    void SetVolume(float volume) override { m_Volume = volume; }
    float GetVolume() const override { return m_Volume; }

private:
    std::string m_Name;
    float m_Volume = 100.0f;
    bool m_Playing = false;
};

class NullAudioStream final : public IAudioStream
{
public:
    NullAudioStream(std::string name) : m_Name(std::move(name)) {}
    ~NullAudioStream() override = default;

    std::string GetName() const override { return m_Name; }

private:
    std::string m_Name;
};

NullAudioSource::NullAudioSource(std::string name) : m_Name(std::move(name))
{
}

void NullAudioSource::SetDefaultVolume(float volume)
{
    m_Volume = ClampPublicVolume(volume);
}

float NullAudioSource::GetDefaultVolume() const
{
    return m_Volume;
}

void NullAudioSource::SetDefaultPitch(float pitch)
{
    m_Pitch = pitch;
}

float NullAudioSource::GetDefaultPitch() const
{
    return m_Pitch;
}

void NullAudioSource::SetDefaultPan(float pan)
{
    m_Pan = pan;
}

float NullAudioSource::GetDefaultPan() const
{
    return m_Pan;
}

void NullAudioSource::SetDefaultSpeed(float speed)
{
    m_Speed = speed;
}

float NullAudioSource::GetDefaultSpeed() const
{
    return m_Speed;
}

std::string NullAudioSource::GetName() const
{
    return m_Name;
}

void NullSound::SetVolume(float volume)
{
    m_Volume = ClampPublicVolume(volume);
}

float NullSound::GetVolume()
{
    return m_Volume;
}

void NullSound::SetPan(float pan)
{
    m_Pan = pan;
}

float NullSound::GetPan()
{
    return m_Pan;
}

void NullSound::SetPitch(float pitch)
{
    m_Pitch = pitch;
}

float NullSound::GetPitch()
{
    return m_Pitch;
}

void NullSound::Stop()
{
    m_Finished = true;
}

void NullSound::Pause()
{
}

void NullSound::Resume()
{
}

bool NullSound::IsFinished()
{
    return m_Finished;
}

void NullSound::SetPosition(const glm::vec3& pos)
{
    m_Position = pos;
}

glm::vec3 NullSound::GetPosition()
{
    return m_Position;
}

void NullSound::SetVelocity(const glm::vec3& vel)
{
    m_Velocity = vel;
}

glm::vec3 NullSound::GetVelocity()
{
    return m_Velocity;
}

void NullSound::SetMinDistance(float minDist)
{
    m_MinDistance = minDist;
}

float NullSound::GetMinDistance()
{
    return m_MinDistance;
}

void NullSound::SetMaxDistance(float maxDist)
{
    m_MaxDistance = maxDist;
}

float NullSound::GetMaxDistance()
{
    return m_MaxDistance;
}

void NullSound::SetIsLooped(bool looped)
{
    m_Looped = looped;
}

bool NullSound::IsLooped()
{
    return m_Looped;
}

void NullSound::SetPlayPosition(unsigned int pos)
{
    m_PlayPosition = pos;
}

unsigned int NullSound::GetPlayPosition()
{
    return m_PlayPosition;
}

unsigned int NullSound::GetPlayLength()
{
    return 0;
}

bool NullAudioEngine::Initialize()
{
    m_Listener = std::make_unique<NullAudioListener>();
    return true;
}

void NullAudioEngine::Update()
{
    m_ActiveSounds.erase(std::remove_if(m_ActiveSounds.begin(), m_ActiveSounds.end(),
                                        [](const std::shared_ptr<NullSound>& sound) { return sound->IsFinished(); }),
                         m_ActiveSounds.end());
}

void NullAudioEngine::Shutdown()
{
    StopAllSounds();
    m_Listener.reset();
}

void NullAudioEngine::SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir)
{
    if (m_Listener)
    {
        m_Listener->SetPosition(pos);
        m_Listener->SetOrientation(lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

void NullAudioEngine::SetGlobalVolume(float volume)
{
    m_GlobalVolume = ClampPublicVolume(volume);
}

std::shared_ptr<ISound> NullAudioEngine::Play2D(const std::string& filename, bool loop, bool startPaused)
{
    auto sound = std::make_shared<NullSound>();
    sound->SetVolume(m_GlobalVolume);
    sound->SetIsLooped(loop);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> NullAudioEngine::Play2D(IAudioSource* source, bool loop, bool startPaused)
{
    auto sound = std::make_shared<NullSound>();
    sound->SetVolume(source ? ApplyGlobalVolume(source->GetDefaultVolume(), m_GlobalVolume) : m_GlobalVolume);
    sound->SetPitch(source ? source->GetDefaultPitch() : 1.0f);
    sound->SetPan(source ? source->GetDefaultPan() : 0.0f);
    sound->SetIsLooped(loop);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> NullAudioEngine::Play3D(const std::string& filename, const glm::vec3& pos, bool loop,
                                                bool startPaused)
{
    auto sound = std::make_shared<NullSound>();
    sound->SetVolume(m_GlobalVolume);
    sound->SetPosition(pos);
    sound->SetIsLooped(loop);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> NullAudioEngine::Play3D(IAudioSource* source, const glm::vec3& pos, bool loop, bool startPaused)
{
    auto sound = std::make_shared<NullSound>();
    sound->SetVolume(source ? ApplyGlobalVolume(source->GetDefaultVolume(), m_GlobalVolume) : m_GlobalVolume);
    sound->SetPitch(source ? source->GetDefaultPitch() : 1.0f);
    sound->SetPan(source ? source->GetDefaultPan() : 0.0f);
    sound->SetPosition(pos);
    sound->SetIsLooped(loop);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<IAudioSource> NullAudioEngine::AddSoundSourceFromFile(const std::string& filename)
{
    return std::make_shared<NullAudioSource>(filename);
}

void NullAudioEngine::StopAllSounds()
{
    for (auto& sound : m_ActiveSounds)
    {
        if (sound)
            sound->Stop();
    }
    m_ActiveSounds.clear();
}

IAudioListener* NullAudioEngine::GetListener()
{
    return m_Listener.get();
}

namespace axis::backend
{
void RegisterNullAudioBackendFactories()
{
    BackendFactoryRegistry::RegisterAudio(
        AudioBackend::Null, [](const AppConfig&) { return std::make_unique<NullAudioEngine>(); });
}
}  // namespace axis::backend
