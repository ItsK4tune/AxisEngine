#include <audio/strategy/fmod/fmod_audio_engine.h>
#include <core/logic/backend_factory_registry.h>
#include <core/logic/logger.h>

#include <fmod.hpp>
#include <fmod_errors.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace
{
constexpr float kMaxPublicVolume = 100.0f;
constexpr float kDefaultMinDistance = 1.0f;
constexpr float kDefaultMaxDistance = 100.0f;

float ClampPublicVolume(float volume)
{
    return std::clamp(volume, 0.0f, kMaxPublicVolume);
}

float ToFMODVolume(float volume)
{
    return ClampPublicVolume(volume) / kMaxPublicVolume;
}

float FromFMODVolume(float volume)
{
    return std::clamp(volume, 0.0f, 1.0f) * kMaxPublicVolume;
}

float SafePitch(float pitch)
{
    return pitch > 0.0f ? pitch : 0.001f;
}

FMOD_VECTOR ToFMODVector(const glm::vec3& value)
{
    return FMOD_VECTOR{value.x, value.y, -value.z};
}

glm::vec3 FromFMODVector(const FMOD_VECTOR& value)
{
    return glm::vec3(value.x, value.y, -value.z);
}

bool IsLostChannelResult(FMOD_RESULT result)
{
    return result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN;
}

bool CheckFMOD(FMOD_RESULT result, const char* operation)
{
    if (result == FMOD_OK)
        return true;

    if (!IsLostChannelResult(result))
        LOGGER_ERROR("FMODAudioEngine") << operation << " failed: " << FMOD_ErrorString(result);

    return false;
}

FMOD_MODE BuildSoundMode(bool is3D, bool loop)
{
    FMOD_MODE mode = FMOD_DEFAULT | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    if (is3D)
        mode |= FMOD_3D | FMOD_3D_WORLDRELATIVE | FMOD_3D_INVERSEROLLOFF;
    else
        mode |= FMOD_2D;
    return mode;
}
}  // namespace

#include <audio/interface/i_audio_listener.h>
#include <audio/interface/i_audio_event.h>
#include <audio/interface/i_audio_stream.h>

class FMODAudioListener final : public IAudioListener
{
public:
    FMODAudioListener(FMOD::System* system) : m_System(system) {}
    ~FMODAudioListener() override = default;

    void SetPosition(const glm::vec3& pos) override
    {
        m_Position = pos;
        UpdateAttributes();
    }

    glm::vec3 GetPosition() const override
    {
        return m_Position;
    }

    void SetOrientation(const glm::vec3& forward, const glm::vec3& up) override
    {
        m_Forward = forward;
        m_Up = up;
        UpdateAttributes();
    }

    glm::vec3 GetForward() const override
    {
        return m_Forward;
    }

    glm::vec3 GetUp() const override
    {
        return m_Up;
    }

    void SetVelocity(const glm::vec3& vel) override
    {
        m_Velocity = vel;
        UpdateAttributes();
    }

    glm::vec3 GetVelocity() const override
    {
        return m_Velocity;
    }

private:
    void UpdateAttributes()
    {
        if (!m_System) return;
        FMOD_VECTOR fmodPos = ToFMODVector(m_Position);
        FMOD_VECTOR fmodVel = ToFMODVector(m_Velocity);
        FMOD_VECTOR fmodForward = ToFMODVector(m_Forward);
        FMOD_VECTOR fmodUp = ToFMODVector(m_Up);
        m_System->set3DListenerAttributes(0, &fmodPos, &fmodVel, &fmodForward, &fmodUp);
    }

    FMOD::System* m_System = nullptr;
    glm::vec3 m_Position{0.0f};
    glm::vec3 m_Forward{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Velocity{0.0f};
};

class FMODAudioEvent final : public IAudioEvent
{
public:
    FMODAudioEvent(std::string name) : m_Name(std::move(name)) {}
    ~FMODAudioEvent() override = default;

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

class FMODAudioStream final : public IAudioStream
{
public:
    FMODAudioStream(std::string name) : m_Name(std::move(name)) {}
    ~FMODAudioStream() override = default;

    std::string GetName() const override { return m_Name; }

private:
    std::string m_Name;
};

class FMODAudioSource final : public IAudioSource
{
public:
    FMODAudioSource(FMOD::Sound* sound, std::string name) : m_Sound(sound), m_Name(std::move(name))
    {
    }

    ~FMODAudioSource() override
    {
        if (m_Sound)
        {
            m_Sound->release();
            m_Sound = nullptr;
        }
    }

    void SetDefaultVolume(float volume) override
    {
        m_Volume = ClampPublicVolume(volume);
    }

    float GetDefaultVolume() const override
    {
        return m_Volume;
    }

    void SetDefaultPitch(float pitch) override
    {
        m_Pitch = SafePitch(pitch);
    }

    float GetDefaultPitch() const override
    {
        return m_Pitch;
    }

    void SetDefaultPan(float pan) override
    {
        m_Pan = std::clamp(pan, -1.0f, 1.0f);
    }

    float GetDefaultPan() const override
    {
        return m_Pan;
    }

    void SetDefaultSpeed(float speed) override
    {
        m_Speed = SafePitch(speed);
    }

    float GetDefaultSpeed() const override
    {
        return m_Speed;
    }

    std::string GetName() const override
    {
        return m_Name;
    }

    FMOD::Sound* GetRaw() const
    {
        return m_Sound;
    }

private:
    FMOD::Sound* m_Sound = nullptr;
    std::string m_Name;
    float m_Volume = kMaxPublicVolume;
    float m_Pitch = 1.0f;
    float m_Pan = 0.0f;
    float m_Speed = 1.0f;
};

class FMODSound final : public ISound
{
public:
    FMODSound(FMOD::Channel* channel, FMOD::Sound* sound, bool ownsSound)
        : m_Channel(channel), m_Sound(sound), m_OwnsSound(ownsSound)
    {
    }

    ~FMODSound() override
    {
        if (m_OwnsSound && m_Sound)
        {
            m_Sound->release();
            m_Sound = nullptr;
        }
    }

    void SetVolume(float volume) override
    {
        m_Volume = ClampPublicVolume(volume);
        if (m_Channel)
            CheckFMOD(m_Channel->setVolume(ToFMODVolume(m_Volume)), "Channel::setVolume");
    }

    float GetVolume() override
    {
        if (!m_Channel)
            return m_Volume;

        float volume = 0.0f;
        if (CheckFMOD(m_Channel->getVolume(&volume), "Channel::getVolume"))
            m_Volume = FromFMODVolume(volume);
        return m_Volume;
    }

    void SetPan(float pan) override
    {
        m_Pan = std::clamp(pan, -1.0f, 1.0f);
        if (m_Channel)
            CheckFMOD(m_Channel->setPan(m_Pan), "Channel::setPan");
    }

    float GetPan() override
    {
        return m_Pan;
    }

    void SetPitch(float pitch) override
    {
        m_Pitch = SafePitch(pitch);
        if (m_Channel)
            CheckFMOD(m_Channel->setPitch(m_Pitch), "Channel::setPitch");
    }

    float GetPitch() override
    {
        if (!m_Channel)
            return m_Pitch;

        float pitch = 1.0f;
        if (CheckFMOD(m_Channel->getPitch(&pitch), "Channel::getPitch"))
            m_Pitch = pitch;
        return m_Pitch;
    }

    void Stop() override
    {
        if (m_Channel)
        {
            CheckFMOD(m_Channel->stop(), "Channel::stop");
            m_Channel = nullptr;
        }

        if (m_OwnsSound && m_Sound)
        {
            CheckFMOD(m_Sound->release(), "Sound::release");
            m_Sound = nullptr;
            m_OwnsSound = false;
        }
    }

    void Pause() override
    {
        if (m_Channel)
            CheckFMOD(m_Channel->setPaused(true), "Channel::setPaused");
    }

    void Resume() override
    {
        if (m_Channel)
            CheckFMOD(m_Channel->setPaused(false), "Channel::setPaused");
    }

    bool IsFinished() override
    {
        if (!m_Channel)
            return true;

        bool playing = false;
        FMOD_RESULT result = m_Channel->isPlaying(&playing);
        if (result == FMOD_OK)
            return !playing;

        if (IsLostChannelResult(result))
        {
            m_Channel = nullptr;
            return true;
        }

        CheckFMOD(result, "Channel::isPlaying");
        return true;
    }

    void SetPosition(const glm::vec3& pos) override
    {
        m_Position = pos;
        if (m_Channel)
        {
            FMOD_VECTOR fmodPos = ToFMODVector(pos);
            CheckFMOD(m_Channel->set3DAttributes(&fmodPos, nullptr), "Channel::set3DAttributes");
        }
    }

    glm::vec3 GetPosition() override
    {
        if (!m_Channel)
            return m_Position;

        FMOD_VECTOR position{};
        FMOD_VECTOR velocity{};
        if (CheckFMOD(m_Channel->get3DAttributes(&position, &velocity), "Channel::get3DAttributes"))
            m_Position = FromFMODVector(position);
        return m_Position;
    }

    void SetVelocity(const glm::vec3& vel) override
    {
        m_Velocity = vel;
        if (m_Channel)
        {
            FMOD_VECTOR fmodVel = ToFMODVector(vel);
            CheckFMOD(m_Channel->set3DAttributes(nullptr, &fmodVel), "Channel::set3DAttributes");
        }
    }

    glm::vec3 GetVelocity() override
    {
        if (!m_Channel)
            return m_Velocity;

        FMOD_VECTOR position{};
        FMOD_VECTOR velocity{};
        if (CheckFMOD(m_Channel->get3DAttributes(&position, &velocity), "Channel::get3DAttributes"))
            m_Velocity = FromFMODVector(velocity);
        return m_Velocity;
    }

    void SetMinDistance(float minDist) override
    {
        m_MinDistance = std::max(0.0f, minDist);
        if (m_Channel)
            CheckFMOD(m_Channel->set3DMinMaxDistance(m_MinDistance, m_MaxDistance), "Channel::set3DMinMaxDistance");
    }

    float GetMinDistance() override
    {
        if (!m_Channel)
            return m_MinDistance;

        float minDistance = m_MinDistance;
        float maxDistance = m_MaxDistance;
        if (CheckFMOD(m_Channel->get3DMinMaxDistance(&minDistance, &maxDistance), "Channel::get3DMinMaxDistance"))
        {
            m_MinDistance = minDistance;
            m_MaxDistance = maxDistance;
        }
        return m_MinDistance;
    }

    void SetMaxDistance(float maxDist) override
    {
        m_MaxDistance = std::max(m_MinDistance, maxDist);
        if (m_Channel)
            CheckFMOD(m_Channel->set3DMinMaxDistance(m_MinDistance, m_MaxDistance), "Channel::set3DMinMaxDistance");
    }

    float GetMaxDistance() override
    {
        if (!m_Channel)
            return m_MaxDistance;

        float minDistance = m_MinDistance;
        float maxDistance = m_MaxDistance;
        if (CheckFMOD(m_Channel->get3DMinMaxDistance(&minDistance, &maxDistance), "Channel::get3DMinMaxDistance"))
        {
            m_MinDistance = minDistance;
            m_MaxDistance = maxDistance;
        }
        return m_MaxDistance;
    }

    void SetIsLooped(bool looped) override
    {
        m_Looped = looped;
        if (m_Channel)
        {
            FMOD_MODE mode = FMOD_DEFAULT;
            if (m_Channel->getMode(&mode) == FMOD_OK)
            {
                mode &= ~(FMOD_LOOP_NORMAL | FMOD_LOOP_OFF | FMOD_LOOP_BIDI);
                mode |= looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
            }
            else
            {
                mode = looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
            }
            CheckFMOD(m_Channel->setMode(mode), "Channel::setMode");
        }
    }

    bool IsLooped() override
    {
        if (!m_Channel)
            return m_Looped;

        FMOD_MODE mode = FMOD_DEFAULT;
        if (CheckFMOD(m_Channel->getMode(&mode), "Channel::getMode"))
            m_Looped = (mode & FMOD_LOOP_NORMAL) != 0;
        return m_Looped;
    }

    void SetPlayPosition(unsigned int pos) override
    {
        m_PlayPosition = pos;
        if (m_Channel)
            CheckFMOD(m_Channel->setPosition(pos, FMOD_TIMEUNIT_MS), "Channel::setPosition");
    }

    unsigned int GetPlayPosition() override
    {
        if (!m_Channel)
            return m_PlayPosition;

        unsigned int position = 0;
        if (CheckFMOD(m_Channel->getPosition(&position, FMOD_TIMEUNIT_MS), "Channel::getPosition"))
            m_PlayPosition = position;
        return m_PlayPosition;
    }

    unsigned int GetPlayLength() override
    {
        FMOD::Sound* sound = m_Sound;
        if (m_Channel)
        {
            FMOD::Sound* currentSound = nullptr;
            if (m_Channel->getCurrentSound(&currentSound) == FMOD_OK && currentSound)
                sound = currentSound;
        }

        if (!sound)
            return 0;

        unsigned int length = 0;
        if (CheckFMOD(sound->getLength(&length, FMOD_TIMEUNIT_MS), "Sound::getLength"))
            return length;
        return 0;
    }

private:
    FMOD::Channel* m_Channel = nullptr;
    FMOD::Sound* m_Sound = nullptr;
    bool m_OwnsSound = false;

    float m_Volume = kMaxPublicVolume;
    float m_Pan = 0.0f;
    float m_Pitch = 1.0f;
    glm::vec3 m_Position{0.0f};
    glm::vec3 m_Velocity{0.0f};
    float m_MinDistance = kDefaultMinDistance;
    float m_MaxDistance = kDefaultMaxDistance;
    bool m_Looped = false;
    unsigned int m_PlayPosition = 0;
};

FMODAudioEngine::FMODAudioEngine()
{
}

FMODAudioEngine::~FMODAudioEngine()
{
    Shutdown();
}

bool FMODAudioEngine::Initialize()
{
    if (!CheckFMOD(FMOD::System_Create(&m_System), "System_Create"))
        return false;

    unsigned int version = 0;
    if (CheckFMOD(m_System->getVersion(&version), "System::getVersion") && version < FMOD_VERSION)
    {
        LOGGER_ERROR("FMODAudioEngine") << "FMOD runtime version is older than the compile-time headers.";
        Shutdown();
        return false;
    }

    if (!CheckFMOD(m_System->init(512, FMOD_INIT_NORMAL, nullptr), "System::init"))
    {
        Shutdown();
        return false;
    }

    CheckFMOD(m_System->set3DSettings(1.0f, 1.0f, 1.0f), "System::set3DSettings");
    CheckFMOD(m_System->getMasterChannelGroup(&m_MasterChannelGroup), "System::getMasterChannelGroup");
    SetGlobalVolume(m_GlobalVolume);

    m_Listener = std::make_unique<FMODAudioListener>(m_System);

    LOGGER_INFO("FMODAudioEngine") << "FMOD initialized.";
    return true;
}

void FMODAudioEngine::Update()
{
    if (m_System)
        CheckFMOD(m_System->update(), "System::update");

    m_ActiveSounds.erase(
        std::remove_if(m_ActiveSounds.begin(), m_ActiveSounds.end(),
                       [](const std::shared_ptr<FMODSound>& sound) { return !sound || sound->IsFinished(); }),
        m_ActiveSounds.end());
}

void FMODAudioEngine::Shutdown()
{
    StopAllSounds();
    m_Sources.clear();
    m_MasterChannelGroup = nullptr;
    m_Listener.reset();

    if (m_System)
    {
        m_System->close();
        m_System->release();
        m_System = nullptr;
    }
}

void FMODAudioEngine::SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir)
{
    if (!m_System)
        return;

    glm::vec3 forward = lookDir;
    if (glm::length(forward) <= 0.0001f)
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    else
        forward = glm::normalize(forward);

    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 up;
    if (glm::length(right) < 0.0001f)
    {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        up = glm::normalize(glm::cross(right, forward));
    }
    else
    {
        right = glm::normalize(right);
        up = glm::normalize(glm::cross(right, forward));
    }

    FMOD_VECTOR fmodPos = ToFMODVector(pos);
    FMOD_VECTOR fmodVel{0.0f, 0.0f, 0.0f};
    FMOD_VECTOR fmodForward = ToFMODVector(forward);
    FMOD_VECTOR fmodUp = ToFMODVector(up);
    CheckFMOD(m_System->set3DListenerAttributes(0, &fmodPos, &fmodVel, &fmodForward, &fmodUp),
              "System::set3DListenerAttributes");
}

void FMODAudioEngine::SetGlobalVolume(float volume)
{
    m_GlobalVolume = ClampPublicVolume(volume);
    if (m_MasterChannelGroup)
        CheckFMOD(m_MasterChannelGroup->setVolume(ToFMODVolume(m_GlobalVolume)), "ChannelGroup::setVolume");
}

std::shared_ptr<ISound> FMODAudioEngine::Play2D(const std::string& filename, bool loop, bool startPaused)
{
    return PlayOwnedSound(filename, false, glm::vec3(0.0f), loop, startPaused);
}

std::shared_ptr<ISound> FMODAudioEngine::Play2D(IAudioSource* source, bool loop, bool startPaused)
{
    auto* fmodSource = dynamic_cast<FMODAudioSource*>(source);
    if (!fmodSource)
        return nullptr;
    return PlaySource(*fmodSource, false, glm::vec3(0.0f), loop, startPaused);
}

std::shared_ptr<ISound> FMODAudioEngine::Play3D(const std::string& filename, const glm::vec3& pos, bool loop,
                                                bool startPaused)
{
    return PlayOwnedSound(filename, true, pos, loop, startPaused);
}

std::shared_ptr<ISound> FMODAudioEngine::Play3D(IAudioSource* source, const glm::vec3& pos, bool loop, bool startPaused)
{
    auto* fmodSource = dynamic_cast<FMODAudioSource*>(source);
    if (!fmodSource)
        return nullptr;
    return PlaySource(*fmodSource, true, pos, loop, startPaused);
}

std::shared_ptr<IAudioSource> FMODAudioEngine::AddSoundSourceFromFile(const std::string& filename)
{
    if (!m_System)
        return nullptr;

    if (auto it = m_Sources.find(filename); it != m_Sources.end())
        return it->second;

    FMOD::Sound* sound = nullptr;
    if (!CheckFMOD(m_System->createSound(filename.c_str(), FMOD_DEFAULT, nullptr, &sound), "System::createSound"))
        return nullptr;

    auto wrapped = std::make_shared<FMODAudioSource>(sound, filename);
    m_Sources[filename] = wrapped;
    return wrapped;
}

void FMODAudioEngine::StopAllSounds()
{
    for (auto& sound : m_ActiveSounds)
    {
        if (sound)
            sound->Stop();
    }
    m_ActiveSounds.clear();

    if (m_System)
        CheckFMOD(m_System->update(), "System::update");
}

std::shared_ptr<ISound> FMODAudioEngine::PlaySource(FMODAudioSource& source, bool is3D, const glm::vec3& pos, bool loop,
                                                    bool startPaused)
{
    if (!m_System || !source.GetRaw())
        return nullptr;

    FMOD::Sound* sound = source.GetRaw();
    CheckFMOD(sound->setMode(BuildSoundMode(is3D, loop)), "Sound::setMode");

    FMOD::Channel* channel = nullptr;
    if (!CheckFMOD(m_System->playSound(sound, nullptr, true, &channel), "System::playSound"))
        return nullptr;

    auto wrapped = std::make_shared<FMODSound>(channel, sound, false);
    wrapped->SetVolume(source.GetDefaultVolume());
    wrapped->SetPitch(source.GetDefaultPitch() * source.GetDefaultSpeed());
    wrapped->SetIsLooped(loop);
    if (is3D)
    {
        wrapped->SetPosition(pos);
        wrapped->SetMinDistance(kDefaultMinDistance);
        wrapped->SetMaxDistance(kDefaultMaxDistance);
    }
    else
    {
        wrapped->SetPan(source.GetDefaultPan());
    }

    if (channel)
        CheckFMOD(channel->setPaused(startPaused), "Channel::setPaused");

    m_ActiveSounds.push_back(wrapped);
    return wrapped;
}

std::shared_ptr<ISound> FMODAudioEngine::PlayOwnedSound(const std::string& filename, bool is3D, const glm::vec3& pos,
                                                        bool loop, bool startPaused)
{
    if (!m_System)
        return nullptr;

    FMOD::Sound* sound = nullptr;
    if (!CheckFMOD(m_System->createSound(filename.c_str(), BuildSoundMode(is3D, loop), nullptr, &sound),
                   "System::createSound"))
    {
        return nullptr;
    }

    FMOD::Channel* channel = nullptr;
    if (!CheckFMOD(m_System->playSound(sound, nullptr, true, &channel), "System::playSound"))
    {
        sound->release();
        return nullptr;
    }

    auto wrapped = std::make_shared<FMODSound>(channel, sound, true);
    wrapped->SetVolume(kMaxPublicVolume);
    wrapped->SetIsLooped(loop);
    if (is3D)
    {
        wrapped->SetPosition(pos);
        wrapped->SetMinDistance(kDefaultMinDistance);
        wrapped->SetMaxDistance(kDefaultMaxDistance);
    }

    if (channel)
        CheckFMOD(channel->setPaused(startPaused), "Channel::setPaused");

    m_ActiveSounds.push_back(wrapped);
    return wrapped;
}

IAudioListener* FMODAudioEngine::GetListener()
{
    return m_Listener.get();
}

namespace axis::backend
{
void RegisterFMODAudioBackendFactories()
{
    BackendFactoryRegistry::RegisterAudio(
        AudioBackend::FMOD, [](const AppConfig&) { return std::make_unique<FMODAudioEngine>(); });
}
}  // namespace axis::backend
