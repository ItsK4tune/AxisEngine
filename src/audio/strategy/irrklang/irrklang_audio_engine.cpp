#include <algorithm>
#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <core/logic/logger.h>

class IrrKlangAudioSource : public IAudioSource
{
public:
    IrrKlangAudioSource(irrklang::ISoundSource* source) : m_Source(source) {}

    void SetDefaultVolume(float volume) override { if(m_Source) m_Source->setDefaultVolume(volume); }
    void SetDefaultMinDistance(float minDist) override { if(m_Source) m_Source->setDefaultMinDistance(minDist); }
    void SetDefaultMaxDistance(float maxDist) override { if(m_Source) m_Source->setDefaultMaxDistance(maxDist); }

    std::string GetName() const override { return m_Source ? m_Source->getName() : ""; }

    irrklang::ISoundSource* GetRaw() const { return m_Source; }

private:
    irrklang::ISoundSource* m_Source = nullptr;
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

    void SetVelocity(const glm::vec3& vel) override
    {
        if(m_Sound) m_Sound->setVelocity(irrklang::vec3df(vel.x, vel.y, vel.z));
    }

    void SetMinDistance(float minDist) override { if(m_Sound) m_Sound->setMinDistance(minDist); }
    void SetMaxDistance(float maxDist) override { if(m_Sound) m_Sound->setMaxDistance(maxDist); }

    void SetIsLooped(bool looped) override { if(m_Sound) m_Sound->setIsLooped(looped); }
    bool IsLooped() override { return m_Sound ? m_Sound->isLooped() : false; }

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

bool IrrKlangAudioEngine::Init()
{
    m_Engine = irrklang::createIrrKlangDevice();
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

    irrklang::ISound* sound = m_Engine->play2D(filename.c_str(), loop, startPaused, true);
    if (sound)
    {
        auto wrapped = std::make_shared<IrrKlangSound>(sound);
        m_ActiveSounds.push_back(wrapped);
        return wrapped;
    }
    return nullptr;
}

std::shared_ptr<ISound> IrrKlangAudioEngine::Play3D(const std::string& filename, const glm::vec3& pos, bool loop, bool startPaused)
{
    if (!m_Engine) return nullptr;

    irrklang::ISound* sound = m_Engine->play3D(filename.c_str(), irrklang::vec3df(pos.x, pos.y, pos.z), loop, startPaused, true);
    if (sound)
    {
        auto wrapped = std::make_shared<IrrKlangSound>(sound);
        m_ActiveSounds.push_back(wrapped);
        return wrapped;
    }
    return nullptr;
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
