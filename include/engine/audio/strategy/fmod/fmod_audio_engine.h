#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <map>
#include <memory>
#include <vector>

namespace FMOD
{
class ChannelGroup;
class System;
}  // namespace FMOD

class FMODAudioSource;
class FMODSound;

class FMODAudioEngine final : public IAudioEngine
{
public:
    FMODAudioEngine();
    ~FMODAudioEngine() override;

    bool Initialize() override;
    void Update() override;
    void Shutdown() override;

    void SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir) override;
    void SetGlobalVolume(float volume) override;

    std::shared_ptr<ISound> Play2D(const std::string& filename, bool loop = false, bool startPaused = false) override;
    std::shared_ptr<ISound> Play2D(IAudioSource* source, bool loop = false, bool startPaused = false) override;
    std::shared_ptr<ISound> Play3D(const std::string& filename, const glm::vec3& pos, bool loop = false,
                                   bool startPaused = false) override;
    std::shared_ptr<ISound> Play3D(IAudioSource* source, const glm::vec3& pos, bool loop = false,
                                   bool startPaused = false) override;

    std::shared_ptr<IAudioSource> AddSoundSourceFromFile(const std::string& filename) override;

    void StopAllSounds() override;

private:
    std::shared_ptr<ISound> PlaySource(FMODAudioSource& source, bool is3D, const glm::vec3& pos, bool loop,
                                       bool startPaused);
    std::shared_ptr<ISound> PlayOwnedSound(const std::string& filename, bool is3D, const glm::vec3& pos, bool loop,
                                           bool startPaused);

    FMOD::System* m_System = nullptr;
    FMOD::ChannelGroup* m_MasterChannelGroup = nullptr;
    std::map<std::string, std::shared_ptr<FMODAudioSource>> m_Sources;
    std::vector<std::shared_ptr<FMODSound>> m_ActiveSounds;
    float m_GlobalVolume = 100.0f;
};
