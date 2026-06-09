#include <audio/strategy/null/null_audio_engine.h>

#include <algorithm>
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
}

void NullAudioEngine::SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir)
{
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
