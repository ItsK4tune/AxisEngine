#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

class ISound
{
public:
    virtual ~ISound() = default;

    virtual void SetVolume(float volume) = 0;
    virtual float GetVolume() = 0;

    virtual void SetPan(float pan) = 0;
    virtual float GetPan() = 0;

    virtual void SetPitch(float pitch) = 0;
    virtual float GetPitch() = 0;

    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual bool IsFinished() = 0;

    virtual void SetPosition(const glm::vec3& pos) = 0;
    virtual glm::vec3 GetPosition() = 0;

    virtual void SetVelocity(const glm::vec3& vel) = 0;
    virtual glm::vec3 GetVelocity() = 0;

    virtual void SetMinDistance(float minDist) = 0;
    virtual float GetMinDistance() = 0;

    virtual void SetMaxDistance(float maxDist) = 0;
    virtual float GetMaxDistance() = 0;

    virtual void SetIsLooped(bool looped) = 0;
    virtual bool IsLooped() = 0;

    virtual void SetPlayPosition(unsigned int pos) = 0;
    virtual unsigned int GetPlayPosition() = 0;
    virtual unsigned int GetPlayLength() = 0;
};