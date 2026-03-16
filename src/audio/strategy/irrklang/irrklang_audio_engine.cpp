#include <algorithm>
#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <core/logic/logger.h>

class IrrKlangAudioSource : public IAudioSource
{
public:
    IrrKlangAudioSource(irrklang::ISoundSource* source) : m_Source(source) {}

    void SetDefaultVolume(float volume) override { if(m_Source) m_Source->setDefaultVolume(volume); }
    float GetDefaultVolume() const override { return m_Source ? m_Source->getDefaultVolume() : 1.0f; }
    
    void SetDefaultPitch(float pitch) override { LOGGER_WARN("IrrKlangAudioSource") << "Default Pitch not supported by irrKlang"; }
    float GetDefaultPitch() const override { return 1.0f; }

    void SetDefaultPan(float pan) override { LOGGER_WARN("IrrKlangAudioSource") << "Default Pan not supported by irrKlang"; }
    float GetDefaultPan() const override { return 0.0f; }

    void SetDefaultSpeed(float speed) override { m_DefaultSpeed = speed; }
    float GetDefaultSpeed() const override { return m_DefaultSpeed; }

    std::string GetName() const override { return m_Source ? m_Source->getName() : ""; }

    irrklang::ISoundSource* GetRaw() const { return m_Source; }

private:
    irrklang::ISoundSource* m_Source = nullptr;
    float m_DefaultSpeed = 1.0f;
};

class IrrKlangSound : public ISound
{
public:
    IrrKlangSound(irrklang::ISound* sound) : m_Sound(sound) {}
    ~IrrKlangSound() { if(m_Sound) { m_Sound->drop(); } }

    void SetVolume(float volume) override { if(m_Sound) m_Sound->setVolume(volume); }
    float GetVolume() override { return m_Sound ? m_Sound->getVolume() : 0.0f; }

    void SetPan(float pan) override { if(m_Sound) m_Sound->setPan(pan); }
    float GetPan() override { return m_Sound ? m_Sound->getPan() : 0.0f; }

    void SetPitch(float pitch) override { if(m_Sound) m_Sound->setPlaybackSpeed(pitch); }
    float GetPitch() override { return m_Sound ? m_Sound->getPlaybackSpeed() : 1.0f; }

    void Stop() override { if(m_Sound) m_Sound->stop(); }
    void Pause() override { if(m_Sound) m_Sound->setIsPaused(true); }
    void Resume() override { if(m_Sound) m_Sound->setIsPaused(false); }
    bool IsFinished() override { return m_Sound ? m_Sound->isFinished() : true; }

    void SetPosition(const glm::vec3& pos) override
    {
        if(m_Sound) m_Sound->setPosition(irrklang::vec3df(pos.x, pos.y, pos.z));
    }
    glm::vec3 GetPosition() override { 
        if (!m_Sound) return glm::vec3(0.0f);
        auto p = m_Sound->getPosition();
        return glm::vec3(p.X, p.Y, p.Z);
    }

    void SetVelocity(const glm::vec3& vel) override
    {
        if(m_Sound) m_Sound->setVelocity(irrklang::vec3df(vel.x, vel.y, vel.z));
    }
    glm::vec3 GetVelocity() override {
        if (!m_Sound) return glm::vec3(0.0f);
        auto v = m_Sound->getVelocity();
        return glm::vec3(v.X, v.Y, v.Z);
    }

    void SetMinDistance(float minDist) override { if(m_Sound) m_Sound->setMinDistance(minDist); }
    float GetMinDistance() override { return m_Sound ? m_Sound->getMinDistance() : 1.0f; }

    void SetMaxDistance(float maxDist) override { if(m_Sound) m_Sound->setMaxDistance(maxDist); }
    float GetMaxDistance() override { return m_Sound ? m_Sound->getMaxDistance() : 100.0f; }

    void SetIsLooped(bool looped) override { if(m_Sound) m_Sound->setIsLooped(looped); }
    bool IsLooped() override { return m_Sound ? m_Sound->isLooped() : false; }

    void SetPlayPosition(unsigned int pos) override { if(m_Sound) m_Sound->setPlayPosition(pos); }
    unsigned int GetPlayPosition() override { return m_Sound ? m_Sound->getPlayPosition() : 0; }
    unsigned int GetPlayLength() override { return m_Sound ? m_Sound->getPlayLength() : 0; }

private:
    irrklang::ISound* m_Sound = nullptr;
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
    m_Engine = irrklang::createIrrKlangDevice(irrklang::ESOD_AUTO_DETECT, 
        irrklang::ESEO_MULTI_THREADED | irrklang::ESEO_LOAD_PLUGINS | irrklang::ESEO_USE_3D_BUFFERS | irrklang::ESEO_PRINT_DEBUG_INFO_TO_DEBUGGER);
    if (!m_Engine)
    {
        LOGGER_ERROR("IrrKlangAudioEngine") << "Failed to create IrrKlang device";
        return false;
    }
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
        m_Engine->setListenerPosition(
            irrklang::vec3df(pos.x, pos.y, pos.z),
            irrklang::vec3df(lookDir.x, lookDir.y, lookDir.z)
        );
    }
}

void IrrKlangAudioEngine::SetGlobalVolume(float volume)
{
    if (m_Engine)
        m_Engine->setSoundVolume(volume);
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play2D(const std::string& filename, bool loop, bool startPaused)
{
    if (!m_Engine) return nullptr;
    auto s = m_Engine->play2D(filename.c_str(), loop, startPaused, true);
    if (!s) return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play2D(IAudioSource* source, bool loop, bool startPaused)
{
    if (!m_Engine || !source) return nullptr;
    auto irrSource = static_cast<IrrKlangAudioSource*>(source);
    auto s = m_Engine->play2D(irrSource->GetRaw(), loop, startPaused, true);
    if (!s) return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play3D(const std::string& filename, const glm::vec3& pos, bool loop, bool startPaused)
{
    if (!m_Engine) return nullptr;
    auto s = m_Engine->play3D(filename.c_str(), irrklang::vec3df(pos.x, pos.y, pos.z), loop, startPaused, true);
    if (!s) return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play3D(IAudioSource* source, const glm::vec3& pos, bool loop, bool startPaused)
{
    if (!m_Engine || !source) return nullptr;
    auto irrSource = static_cast<IrrKlangAudioSource*>(source);
    auto s = m_Engine->play3D(irrSource->GetRaw(), irrklang::vec3df(pos.x, pos.y, pos.z), loop, startPaused, true);
    if (!s) return nullptr;
    auto sound = std::make_shared<IrrKlangSound>(s);
    m_ActiveSounds.push_back(sound);
    return sound;
}

std::shared_ptr<IAudioSource> IrrKlangAudioEngine::AddSoundSourceFromFile(const std::string& filename)
{
    if (!m_Engine) return nullptr;

    if (m_Sources.find(filename) != m_Sources.end())
        return m_Sources[filename];

    irrklang::ISoundSource* source = m_Engine->addSoundSourceFromFile(filename.c_str());
    if (source)
    {
        auto wrapped = std::make_shared<IrrKlangAudioSource>(source);
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
