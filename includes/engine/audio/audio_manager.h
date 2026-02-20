#pragma once

#include <memory>
#include <memory>
#include <interface/audio/i_audio_engine.h>
#include <interface/window/i_window.h>
#include <interface/window/i_device_manager.h>

class AudioManager : public IDeviceManager
{
public:
    AudioManager(std::unique_ptr<IAudioEngine> engine);
    ~AudioManager();

    bool Init();
    void Shutdown();

    IAudioEngine* GetEngine() const { return m_Engine.get(); }

    std::vector<DeviceInfo> GetAllDevices() const override { return {}; }
    DeviceInfo GetCurrentDevice() const override { return {"Default", "Default", AxisDeviceType::AudioOutput, true}; }
    bool SetActiveDevice(const std::string& deviceId) override { return false; }

private:
    std::unique_ptr<IAudioEngine> m_Engine;
};
