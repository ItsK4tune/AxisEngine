# Microphone capture

> [Tiếng Việt](../../vi/guides/audio_capture.md)

Microphone input is independent from audio playback. Axis registers an `IAudioCaptureService`; Windows uses WASAPI shared-mode capture and other platforms expose an explicit unsupported null service until a capture backend is supplied.

## Configuration

```yaml
axis_config:
  AUDIO_CAPTURE_ENABLED: 1
  AUDIO_CAPTURE_DEVICE: "" # empty selects the default endpoint
  AUDIO_CAPTURE_INPUT_VOLUME: 1.0 # software pre-amplifier, 0.0-4.0
  AUDIO_CAPTURE_NOISE_GATE: 0.02
  AUDIO_CAPTURE_GAIN: 4.0
  AUDIO_CAPTURE_ATTACK_SECONDS: 0.05
  AUDIO_CAPTURE_RELEASE_SECONDS: 0.05
  AUDIO_CAPTURE_PEAK_DECAY_SECONDS: 0.125
  AUDIO_CAPTURE_CALIBRATION_SECONDS: 1.0
  AUDIO_CAPTURE_PULSE_THRESHOLD: 0.15
  AUDIO_CAPTURE_PULSE_COOLDOWN: 0.08
  AUDIO_CAPTURE_PULSE_DURATION: 0.6
```

`MIC_INPUT_VOLUME` and `MIC_INPUT_THRESHOLD` are accepted as aliases for `AUDIO_CAPTURE_INPUT_VOLUME` and `AUDIO_CAPTURE_NOISE_GATE`. Input volume is applied before calibration and gating; gain is applied afterward to the normalized voice intensity. Attack/release control RMS response, while peak decay controls how long transients remain visible.

At startup and on request, calibration samples ambient RMS to establish the noise floor. Each real-time update computes RMS, a decaying peak, gated intensity, and bounded pulses. `GetSnapshot()` returns a thread-safe copy for gameplay code.

## Replacing the backend

Implement `IAudioCaptureService`, install it with `AppBuilder::WithAudioCaptureFactory`, and pass that builder to the `Application` constructor. A miniaudio or FMOD implementation can feed raw RMS/peak values into `AudioCaptureProcessor`, which provides shared sanitization, smoothing, time-weighted calibration, noise gating, and bounded pulse lifecycle. Synchronization remains the owning service's responsibility when capture and frame updates use different threads. Playback remains behind `IAudioEngine`; recording is intentionally a separate service.

Custom post-process shaders receive the current level through `u_AudioLevel` and pulses through SSBO binding `26`; see [Graphics & Rendering Guide](graphics.md#9-custom-post-process-abi).
