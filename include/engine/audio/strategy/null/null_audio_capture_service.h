#pragma once

#include <audio/interface/i_audio_capture_service.h>
#include <audio/logic/audio_capture_processor.h>

class NullAudioCaptureService final : public IAudioCaptureService
{
public:
    bool Initialize(const AudioCaptureSettings& settings) override;
    void Shutdown() override;
    bool RefreshDevices() override;
    std::vector<AudioCaptureDevice> GetDevices() const override;
    AudioCaptureResult Start(const std::string& deviceId = {}) override;
    void Stop() override;
    bool IsCapturing() const override;
    void Update(float deltaTime) override;
    void BeginCalibration(float seconds) override;
    void SetSettings(const AudioCaptureSettings& settings) override;
    AudioCaptureSettings GetSettings() const override;
    AudioCaptureSnapshot GetSnapshot() const override;

private:
    AudioCaptureProcessor m_Processor;
};
