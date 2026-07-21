#pragma once

#include <audio/interface/i_audio_capture_service.h>

// Platform-independent level processing for audio-capture backends. The
// owning service is responsible for synchronization when raw capture and
// frame updates happen on different threads.
class AudioCaptureProcessor
{
public:
    explicit AudioCaptureProcessor(const AudioCaptureSettings& settings = {});

    static AudioCaptureSettings SanitizeSettings(AudioCaptureSettings settings);

    void SetSettings(const AudioCaptureSettings& settings);
    const AudioCaptureSettings& GetSettings() const;

    void BeginCalibration(float seconds);
    void Update(float deltaTime, bool isCapturing, float rawRms, float rawPeak);
    void ResetLevelState();

    const AudioCaptureSnapshot& GetSnapshot() const;

private:
    AudioCaptureSettings m_Settings;
    AudioCaptureSnapshot m_Snapshot;
    float m_CalibrationRemaining = 0.0f;
    float m_CalibrationWeightedTotal = 0.0f;
    float m_CalibrationElapsed = 0.0f;
    float m_PulseCooldownRemaining = 0.0f;
};
