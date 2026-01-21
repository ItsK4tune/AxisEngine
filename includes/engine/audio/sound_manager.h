#pragma once

#include <irrKlang/irrKlang.h>
#include <glm/glm.hpp>
#include <iostream>
#include <interface/device_manager.h>

using namespace irrklang;

class SoundManager : public IDeviceManager
{
public:
    ~SoundManager();

    void Init();

    void Play2D(const char *path, bool loop = false);
    ISound *Play2D(ISoundSource *source, bool loop = false);
    ISound *Play3D(ISoundSource *source, glm::vec3 pos, bool loop = false);

    void UpdateListener(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up);

    void SetVolume(float volume);
    void SetVolume(ISoundSource *source, float volume);

    void Stop(ISoundSource *source);
    void StopAll();

    ISoundEngine *GetEngine() const { return m_Engine; }

    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string &deviceId) override;

private:
    ISoundEngine *m_Engine = nullptr;
};