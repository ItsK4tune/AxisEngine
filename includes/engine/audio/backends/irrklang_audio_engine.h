#pragma once

#include <audio/interfaces/i_audio_engine.h>
#include <audio/interfaces/i_audio_source.h>
#include <audio/interfaces/i_sound.h>
#include <irrKlang/irrKlang.h>
#include <map>
#include <memory>
#include <vector>

class IrrKlangAudioSource;
class IrrKlangSound;

class IrrKlangAudioEngine : public IAudioEngine
{
public:
    IrrKlangAudioEngine();
    ~IrrKlangAudioEngine() override;

    bool Init() override;
    void Update() override;
    void Shutdown() override;

    void SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir) override;
    void SetGlobalVolume(float volume) override;

    std::shared_ptr<ISound> Play2D(const std::string& filename, bool loop = false, bool startPaused = false) override;
    std::shared_ptr<ISound> Play3D(const std::string& filename, const glm::vec3& pos, bool loop = false, bool startPaused = false) override;

    std::shared_ptr<IAudioSource> AddSoundSourceFromFile(const std::string& filename) override;

    void StopAllSounds() override;

private:
    irrklang::ISoundEngine* m_Engine = nullptr;
    std::map<std::string, std::shared_ptr<IrrKlangAudioSource>> m_Sources;
    std::vector<std::shared_ptr<IrrKlangSound>> m_ActiveSounds;
};
