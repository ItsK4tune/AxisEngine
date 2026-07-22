#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <core/logic/logger.h>
#include <algorithm>

namespace
{
constexpr float kMaxPublicVolume = 100.0f;

float ToIrrKlangVolume(float volume)
{
    return std::clamp(volume, 0.0f, kMaxPublicVolume) / kMaxPublicVolume;
}

float FromIrrKlangVolume(float volume)
{
    return std::clamp(volume, 0.0f, 1.0f) * kMaxPublicVolume;
}
}  // namespace

class IrrKlangAudioSource : public IAudioSource
{
public:
    IrrKlangAudioSource(irrklang::ISoundSource* source, std::weak_ptr<std::atomic_bool> lifetime)
        : m_Source(source), m_Lifetime(std::move(lifetime))
    {
    }

    bool IsValid() const
    {
        const auto lifetime = m_Lifetime.lock();
        return m_Source && lifetime && lifetime->load(std::memory_order_acquire);
    }

    void SetDefaultVolume(float volume) override
    {
        if (IsValid())
            m_Source->setDefaultVolume(ToIrrKlangVolume(volume));
    }
    float GetDefaultVolume() const override
    {
        return IsValid() ? FromIrrKlangVolume(m_Source->getDefaultVolume()) : kMaxPublicVolume;
    }

    void SetDefaultPitch(float pitch) override
    {
        LOGGER_WARN("IrrKlangAudioSource") << "Default Pitch not supported by irrKlang";
    }
    float GetDefaultPitch() const override
    {
        return 1.0f;
    }
    bool SupportsDefaultPitch() const override
    {
        return false;
    }

    void SetDefaultPan(float pan) override
    {
        LOGGER_WARN("IrrKlangAudioSource") << "Default Pan not supported by irrKlang";
    }
    float GetDefaultPan() const override
    {
        return 0.0f;
    }
    bool SupportsDefaultPan() const override
    {
        return false;
    }

    void SetDefaultSpeed(float speed) override
    {
        m_DefaultSpeed = speed;
    }
    float GetDefaultSpeed() const override
    {
        return m_DefaultSpeed;
    }

    std::string GetName() const override
    {
        return IsValid() ? m_Source->getName() : "";
    }

    irrklang::ISoundSource* GetRaw() const
    {
        return IsValid() ? m_Source : nullptr;
    }

private:
    irrklang::ISoundSource* m_Source = nullptr;
    std::weak_ptr<std::atomic_bool> m_Lifetime;
    float m_DefaultSpeed = 1.0f;
};

class IrrKlangSound : public ISound
{
public:
    IrrKlangSound(irrklang::ISound* sound, std::weak_ptr<std::atomic_bool> lifetime)
        : m_Sound(sound), m_Lifetime(std::move(lifetime))
    {
    }
    ~IrrKlangSound()
    {
        if (IsValid())
        {
            m_Sound->drop();
        }
    }

    bool IsValid() const
    {
        const auto lifetime = m_Lifetime.lock();
        return m_Sound && lifetime && lifetime->load(std::memory_order_acquire);
    }

    void SetVolume(float volume) override
    {
        if (IsValid())
            m_Sound->setVolume(ToIrrKlangVolume(volume));
    }
    float GetVolume() override
    {
        return IsValid() ? FromIrrKlangVolume(m_Sound->getVolume()) : 0.0f;
    }

    void SetPan(float pan) override
    {
        if (IsValid())
            m_Sound->setPan(pan);
    }
    float GetPan() override
    {
        return IsValid() ? m_Sound->getPan() : 0.0f;
    }

    void SetPitch(float pitch) override
    {
        if (IsValid())
            m_Sound->setPlaybackSpeed(pitch);
    }
    float GetPitch() override
    {
        return IsValid() ? m_Sound->getPlaybackSpeed() : 1.0f;
    }

    void Stop() override
    {
        if (IsValid())
            m_Sound->stop();
    }
    void Pause() override
    {
        if (IsValid())
            m_Sound->setIsPaused(true);
    }
    void Resume() override
    {
        if (IsValid())
            m_Sound->setIsPaused(false);
    }
    bool IsFinished() override
    {
        return IsValid() ? m_Sound->isFinished() : true;
    }

    void SetPosition(const glm::vec3& pos) override
    {
        if (IsValid())
            m_Sound->setPosition(irrklang::vec3df(pos.x, pos.y, -pos.z));
    }
    glm::vec3 GetPosition() override
    {
        if (!IsValid())
            return glm::vec3(0.0f);
        auto p = m_Sound->getPosition();
        return glm::vec3(p.X, p.Y, -p.Z);
    }

    void SetVelocity(const glm::vec3& vel) override
    {
        if (IsValid())
            m_Sound->setVelocity(irrklang::vec3df(vel.x, vel.y, -vel.z));
    }
    glm::vec3 GetVelocity() override
    {
        if (!IsValid())
            return glm::vec3(0.0f);
        auto v = m_Sound->getVelocity();
        return glm::vec3(v.X, v.Y, -v.Z);
    }

    void SetMinDistance(float minDist) override
    {
        if (IsValid())
            m_Sound->setMinDistance(minDist);
    }
    float GetMinDistance() override
    {
        return IsValid() ? m_Sound->getMinDistance() : 1.0f;
    }

    void SetMaxDistance(float maxDist) override
    {
        if (IsValid())
            m_Sound->setMaxDistance(maxDist);
    }
    float GetMaxDistance() override
    {
        return IsValid() ? m_Sound->getMaxDistance() : 100.0f;
    }

    void SetIsLooped(bool looped) override
    {
        if (IsValid())
            m_Sound->setIsLooped(looped);
    }
    bool IsLooped() override
    {
        return IsValid() ? m_Sound->isLooped() : false;
    }

    void SetPlayPosition(unsigned int pos) override
    {
        if (IsValid())
            m_Sound->setPlayPosition(pos);
    }
    unsigned int GetPlayPosition() override
    {
        return IsValid() ? m_Sound->getPlayPosition() : 0;
    }
    unsigned int GetPlayLength() override
    {
        return IsValid() ? m_Sound->getPlayLength() : 0;
    }

private:
    irrklang::ISound* m_Sound = nullptr;
    std::weak_ptr<std::atomic_bool> m_Lifetime;
};

IrrKlangAudioEngine::IrrKlangAudioEngine()
{
}

IrrKlangAudioEngine::~IrrKlangAudioEngine()
{
    Shutdown();
}

bool IrrKlangAudioEngine::Initialize()
{
    if (!m_Lifetime || m_Lifetime->load(std::memory_order_acquire))
        m_Lifetime = std::make_shared<std::atomic_bool>(false);
    m_Engine = irrklang::createIrrKlangDevice(
        irrklang::ESOD_AUTO_DETECT, irrklang::ESEO_MULTI_THREADED | irrklang::ESEO_LOAD_PLUGINS |
                                        irrklang::ESEO_USE_3D_BUFFERS | irrklang::ESEO_PRINT_DEBUG_INFO_TO_DEBUGGER);
    if (!m_Engine)
    {
        LOGGER_ERROR("IrrKlangAudioEngine") << "Failed to create IrrKlang device";
        return false;
    }
    m_Lifetime->store(true, std::memory_order_release);
    return true;
}

void IrrKlangAudioEngine::Update()
{
    m_ActiveSounds.erase(
        std::remove_if(m_ActiveSounds.begin(), m_ActiveSounds.end(),
                       [](const std::shared_ptr<IrrKlangSound>& sound) { return sound->IsFinished(); }),
        m_ActiveSounds.end());
}

void IrrKlangAudioEngine::Shutdown()
{
    StopAllSounds();
    if (m_Lifetime)
        m_Lifetime->store(false, std::memory_order_release);
    if (m_Engine)
    {
        m_Engine->drop();
        m_Engine = nullptr;
    }
    m_Sources.clear();
}

void IrrKlangAudioEngine::SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir)
{
    if (m_Engine)
    {
        m_Engine->setListenerPosition(irrklang::vec3df(pos.x, pos.y, -pos.z),
                                      irrklang::vec3df(lookDir.x, lookDir.y, -lookDir.z));
    }
}

void IrrKlangAudioEngine::SetGlobalVolume(float volume)
{
    if (m_Engine)
        m_Engine->setSoundVolume(ToIrrKlangVolume(volume));
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play2D(const std::string& filename, bool loop, bool startPaused)
{
    if (!m_Engine)
        return nullptr;
    auto s = m_Engine->play2D(filename.c_str(), loop, startPaused, true);
    if (!s)
        return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s, m_Lifetime);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play2D(IAudioSource* source, bool loop, bool startPaused)
{
    if (!m_Engine || !source)
        return nullptr;
    auto* irrSource = dynamic_cast<IrrKlangAudioSource*>(source);
    if (!irrSource || !irrSource->GetRaw())
    {
        LOGGER_ERROR("IrrKlangAudioEngine") << "Play2D received an audio source owned by another backend";
        return nullptr;
    }
    auto s = m_Engine->play2D(irrSource->GetRaw(), loop, startPaused, true);
    if (!s)
        return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s, m_Lifetime);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play3D(const std::string& filename, const glm::vec3& pos, bool loop,
                                                    bool startPaused)
{
    if (!m_Engine)
        return nullptr;
    auto s = m_Engine->play3D(filename.c_str(), irrklang::vec3df(pos.x, pos.y, -pos.z), loop, startPaused, true);
    if (!s)
        return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s, m_Lifetime);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play3D(IAudioSource* source, const glm::vec3& pos, bool loop,
                                                    bool startPaused)
{
    if (!m_Engine || !source)
        return nullptr;
    auto* irrSource = dynamic_cast<IrrKlangAudioSource*>(source);
    if (!irrSource || !irrSource->GetRaw())
    {
        LOGGER_ERROR("IrrKlangAudioEngine") << "Play3D received an audio source owned by another backend";
        return nullptr;
    }
    auto s = m_Engine->play3D(irrSource->GetRaw(), irrklang::vec3df(pos.x, pos.y, -pos.z), loop, startPaused, true);
    if (!s)
        return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s, m_Lifetime);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<IAudioSource> IrrKlangAudioEngine::AddSoundSourceFromFile(const std::string& filename)
{
    if (!m_Engine)
        return nullptr;

    if (m_Sources.find(filename) != m_Sources.end())
        return m_Sources[filename];

    irrklang::ISoundSource* source = m_Engine->addSoundSourceFromFile(filename.c_str());
    if (source)
    {
        auto wrapped = std::make_shared<IrrKlangAudioSource>(source, m_Lifetime);
        m_Sources[filename] = wrapped;
        return wrapped;
    }
    return nullptr;
}

void IrrKlangAudioEngine::StopAllSounds()
{
    if (m_Engine)
        m_Engine->stopAllSounds();

    m_ActiveSounds.clear();
}
