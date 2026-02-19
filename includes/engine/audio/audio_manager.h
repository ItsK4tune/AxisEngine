#pragma once

#include <memory>
#include <memory>
#include <interface/audio/i_audio_engine.h>
#include <interface/window/i_device_manager.h>

class AudioManager : public IDeviceManager
{
public:
    AudioManager(std::unique_ptr<IAudioEngine> engine);
    ~AudioManager();

    bool Init();
    void Shutdown();

    IAudioEngine* GetEngine() const { return m_Engine.get(); }

    // IDeviceManager methods - Audio device enumeration might need IAudioEngine support or separate logic
    // For now, IAudioEngine doesn't support device enumeration in my interface. 
    // I will keep IDeviceManager inheritance but stub implementation or add to IAudioEngine later if needed.
    // Given the task is abstraction, I will remove device enumeration for now from AudioManager unless I add it to IAudioEngine.
    // simpler to keep it compatible with existing code if possible.
    // IAudioBackend had EnumerateDevices and InitWithDevice. IAudioEngine does not (yet).
    // I should probably add those to IAudioEngine or just simplify for now.
    // I'll stick to a simpler IAudioEngine for now and comment out IDeviceManager parts if they depend on backend specific features not yet in interface.
    // Actually, I'll remove IDeviceManager inheritance for now to simplify abstraction if it relies on removed backend interface.
    // Or better, I will update IAudioEngine to include device management if important.
    // For this step, I will focus on the core audio playing abilities.

    std::vector<DeviceInfo> GetAllDevices() const override { return {}; } // Stubbed
    DeviceInfo GetCurrentDevice() const override { return {"Default", "Default", DeviceType::AudioOutput, true}; } // Stubbed
    bool SetActiveDevice(const std::string& deviceId) override { return false; } // Stubbed

private:
    std::unique_ptr<IAudioEngine> m_Engine;
};
