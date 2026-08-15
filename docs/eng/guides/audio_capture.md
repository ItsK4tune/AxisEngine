# Audio Capture Guide (WASAPI Microphone)

> [Tiếng Việt](../../vi/guides/audio_capture.md) | [Audio System](audio.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine integrates native **WASAPI Microphone Audio Capture** on Windows systems. Managed via `IAudioCaptureService`, the capture subsystem processes real-time microphone input, applying pre-amplification gain, noise-gate filtering, ambient calibration, and pulse detection for voice-reactive game mechanics.

---

## 2. How to Use

1. **Enable Audio Capture**: Set `AUDIO_CAPTURE_ENABLED: 1` in `axis_config.axs`.
2. **Access Service**: Retrieve service instance via `ServiceLocator::Get<IAudioCaptureService>()`.
3. **Query Volume Levels**: Call `capture->GetNormalizedLevel()` (returns `0.0` to `1.0`).
4. **Subscribe to Events**: Register a listener for `VoicePulseEvent` via `EventManager`.

---

## 3. Examples

### 1. Monitoring Microphone Volume Level Example
```cpp
#include <axis_sdk.h>

void UpdateMicVolumeMeter() {
    auto capture = ServiceLocator::Get<IAudioCaptureService>();
    if (capture && capture->IsCapturing()) {
        float level = capture->GetNormalizedLevel();
        if (level > 0.6f) {
            AXIS_LOG_INFO("Voice input peak detected: " + std::to_string(level));
        }
    }
}
```

### 2. Subscribing to Voice Pulse Trigger Event Example
```cpp
#include <axis_sdk.h>

struct VoicePulseEvent { float peakAmplitude; };

void SetupVoiceSpells() {
    EventManager::Get().Subscribe<VoicePulseEvent>([](const VoicePulseEvent& e) {
        AXIS_LOG_INFO("Voice spell cast triggered with amplitude: " + std::to_string(e.peakAmplitude));
    });
}
```

---

## 4. API & Configuration Reference

### Audio Capture Settings Reference Table

| Setting Key | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `AUDIO_CAPTURE_ENABLED` | `bool` | `0` | Enables microphone capture device startup |
| `AUDIO_CAPTURE_DEVICE` | `string` | `""` | Specific device ID (empty uses system default) |
| `AUDIO_CAPTURE_INPUT_VOLUME`| `float` | `1.0` | Input pre-amplification scaling |
| `AUDIO_CAPTURE_NOISE_GATE` | `float` | `0.05` | Minimum amplitude threshold below which signal is cut |
| `AUDIO_CAPTURE_GAIN` | `float` | `1.0` | Post-gate output amplification multiplier |
| `AUDIO_CAPTURE_PULSE_THRESHOLD`| `float` | `0.4` | Peak amplitude threshold for triggering voice pulse events |
