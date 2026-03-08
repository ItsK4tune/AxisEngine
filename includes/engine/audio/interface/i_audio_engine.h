#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class IAudioSource;
class ISound;

#define GLM_ENABLE_EXPERIMENTAL


class IAudioEngine
{
public:
    virtual ~IAudioEngine() = default;

    virtual bool Init() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;

    virtual void SetListenerPosition(const glm::vec3& pos, const glm::vec3& lookDir) = 0;
    virtual void SetGlobalVolume(float volume) = 0;

    virtual std::shared_ptr<ISound> Play2D(const std::string& filename, bool loop = false, bool startPaused = false) = 0;
    virtual std::shared_ptr<ISound> Play3D(const std::string& filename, const glm::vec3& pos, bool loop = false, bool startPaused = false) = 0;

    virtual std::shared_ptr<IAudioSource> AddSoundSourceFromFile(const std::string& filename) = 0;

    virtual void StopAllSounds() = 0;
};