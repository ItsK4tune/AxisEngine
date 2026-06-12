#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <vector>

class NullAudioSource final : public IAudioSource
{
public:
    explicit NullAudioSource(std::string name = {});

    void SetDefaultVolume(float volume) override;
    float GetDefaultVolume() const override;
    void SetDefaultPitch(float pitch) override;
    float GetDefaultPitch() const override;
    void SetDefaultPan(float pan) override;
    float GetDefaultPan() const override;
    void SetDefaultSpeed(float speed) override;
    float GetDefaultSpeed() const override;
    std::string GetName() const override;

private:
    std::string m_Name;
    float m_Volume = 100.0f;
    float m_Pitch = 1.0f;
    float m_Pan = 0.0f;
    float m_Speed = 1.0f;
};

class NullSound final : public ISound
{
public:
    void SetVolume(float volume) override;
    float GetVolume() override;
    void SetPan(float pan) override;
    float GetPan() override;
    void SetPitch(float pitch) override;
    float GetPitch() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    bool IsFinished() override;
    void SetPosition(const glm::vec3& pos) override;
    glm::vec3 GetPosition() override;
    void SetVelocity(const glm::vec3& vel) override;
    glm::vec3 GetVelocity() override;
    void SetMinDistance(float minDist) override;
    float GetMinDistance() override;
    void SetMaxDistance(float maxDist) override;
    float GetMaxDistance() override;
    void SetIsLooped(bool looped) override;
    bool IsLooped() override;
    void SetPlayPosition(unsigned int pos) override;
    unsigned int GetPlayPosition() override;
    unsigned int GetPlayLength() override;

private:
    float m_Volume = 100.0f;
    float m_Pan = 0.0f;
    float m_Pitch = 1.0f;
    glm::vec3 m_Position{0.0f};
    glm::vec3 m_Velocity{0.0f};
    float m_MinDistance = 1.0f;
    float m_MaxDistance = 100.0f;
    bool m_Looped = false;
    bool m_Finished = false;
    unsigned int m_PlayPosition = 0;
};

class NullAudioEngine final : public IAudioEngine
{
public:
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
    IAudioListener* GetListener() override;

private:
    std::unique_ptr<IAudioListener> m_Listener;
    std::vector<std::shared_ptr<NullSound>> m_ActiveSounds;
    float m_GlobalVolume = 100.0f;
};
