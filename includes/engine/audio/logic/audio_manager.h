#pragma once

#include <audio/interface/i_audio_engine.h>
#include <memory>
#include <platform/interface/i_device_manager.h>
#include <platform/interface/i_window.h>

class AudioManager : public IDeviceManager
{
public:
    AudioManager(std::unique_ptr<IAudioEngine> engine);
    ~AudioManager();

    bool Init();
    void Shutdown();

    IAudioEngine* GetEngine() const { return m_Engine.get(); }

    std::vector<DeviceInfo> GetAllDevices() const override { return {}; }
    DeviceInfo GetCurrentDevice() const override { return {"Default", "Default", DeviceType::AudioOutput, true}; }
    bool SetActiveDevice(const std::string& deviceId) override { return false; }

private:
    std::unique_ptr<IAudioEngine> m_Engine;
};