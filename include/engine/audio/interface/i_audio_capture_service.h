#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace AudioCaptureLimits
{
inline constexpr size_t MaxPulses = 64;
}

enum class AudioCaptureResult
{
    Success,
    AlreadyRunning,
    Unsupported,
    DeviceNotFound,
    PermissionDenied,
    BackendError
};

struct AudioCaptureDevice
{
    std::string id;
    std::string name;
    bool isDefault = false;
};

struct AudioCaptureSettings
{
    // Software pre-amplifier applied to raw RMS/peak samples before
    // calibration, smoothing, gating, and pulse detection.
    float inputVolume = 1.0f;
    float noiseGate = 0.02f;
    float gain = 4.0f;
    float attackSeconds = 0.05f;
    float releaseSeconds = 0.05f;
    float peakDecaySeconds = 0.125f;
    float calibrationSeconds = 1.0f;
    float pulseThreshold = 0.15f;
    float pulseCooldown = 0.08f;
    float pulseDuration = 0.6f;
};

struct AudioCaptureLevel
{
    float rms = 0.0f;
    float peak = 0.0f;
    float intensity = 0.0f;
    float noiseFloor = 0.0f;
};

// std430-compatible representation used by custom post-process shaders.
struct alignas(16) AudioPulse
{
    float intensity = 0.0f;
    float peak = 0.0f;
    float age = 0.0f;
    float duration = 0.0f;
};
static_assert(sizeof(AudioPulse) == 16, "AudioPulse must match the std430 shader layout");

struct AudioCaptureSnapshot
{
    AudioCaptureLevel level;
    std::vector<AudioPulse> pulses;
};

class IAudioCaptureService
{
public:
    virtual ~IAudioCaptureService() = default;

    virtual bool Initialize(const AudioCaptureSettings& settings) = 0;
    virtual void Shutdown() = 0;

    virtual bool RefreshDevices() = 0;
    virtual std::vector<AudioCaptureDevice> GetDevices() const = 0;
    // An empty device id selects the platform default capture endpoint.
    virtual AudioCaptureResult Start(const std::string& deviceId = {}) = 0;
    virtual void Stop() = 0;
    virtual bool IsCapturing() const = 0;

    // Called once per frame with unscaled real time.
    virtual void Update(float deltaTime) = 0;
    virtual void BeginCalibration(float seconds) = 0;
    virtual void SetSettings(const AudioCaptureSettings& settings) = 0;
    virtual AudioCaptureSettings GetSettings() const = 0;
    virtual AudioCaptureSnapshot GetSnapshot() const = 0;
};
